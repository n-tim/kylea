#include "ClientHTTPSession.hpp"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <openssl/ssl3.h>

#include <Buffer.h>

#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message()

using namespace Net;

ClientHTTPSession::ClientHTTPSession(boost::asio::io_context& ioService, boost::asio::ssl::context& context)
  : socket_(ioService, context)
  , socketStrand_(boost::asio::make_strand(ioService))
  , readStrand_(boost::asio::make_strand(ioService))
{
  LOG_TRACE << "session created" << std::endl;

  isConnected_ = false;
}

ClientHTTPSession::~ClientHTTPSession()
{
  LOG_TRACE << "session deleted" << std::endl;
}

void ClientHTTPSession::configure(const OpenHandler& openHandler, const CloseHandler& closeHandler, const ReceiveHandler& receiveHandler)
{
  openHandler_ = openHandler;
  closeHandler_ = closeHandler;
  receiveHandler_ = receiveHandler;
}

void ClientHTTPSession::drop()
{
  close();
}

void ClientHTTPSession::open()
{
  if (isConnected_)
  {
    LOG_TRACE << "open: already opened" << std::endl;
    return;
  }

  configureSocket();

  isConnected_ = true;

  auto self = shared_from_this();
  boost::asio::post(socketStrand_, [this, self]()
  {
    asyncHandshake();
  });
}

void ClientHTTPSession::cancelAllPendingOperations()
{
  auto self = shared_from_this();
  boost::asio::post(socketStrand_, [this, self]()
  {
    boost::system::error_code error;
    socket().cancel(error);

    LOG_TRACE_IF_ERROR(error, "cancelAllPendingOperations");
  });
}

void ClientHTTPSession::close()
{
  cancelAllPendingOperations();

  auto self = shared_from_this();

  if (!isConnected_)
  {
    LOG_TRACE << "close: already closed" << std::endl;

    return;
  }

  isConnected_ = false;
  boost::asio::post(socketStrand_, [this, self]()
  {
    {
      std::lock_guard<std::recursive_mutex> lock(queueMutex_);
      writeQueue_.clear();
    }

    boost::system::error_code error;
    socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    socket().close(error);

    if (closeHandler_)
    {
      closeHandler_(self);
    }

    LOG_TRACE << "close: socket closed" << std::endl;
  });
}

boost::asio::ip::tcp::socket& ClientHTTPSession::socket()
{
  return socket_.next_layer();
}

void ClientHTTPSession::configureSocket()
{
  boost::system::error_code error;

  {
    socket().set_option(boost::asio::ip::tcp::no_delay(true));
  }
}

void ClientHTTPSession::readNext()
{
  auto message = std::make_shared<std::string>();
  message->resize(1024 * 10);
  asyncRead(message);
}

void ClientHTTPSession::asyncHandshake()
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  socket_.async_handshake(boost::asio::ssl::stream_base::client,
    boost::asio::bind_executor(socketStrand_, boost::bind(&ClientHTTPSession::handleHandshake, shared_from_this(), boost::asio::placeholders::error)));
}

void ClientHTTPSession::handleHandshake(const boost::system::error_code& error)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  if (!error)
  {
    readNext();

    if (openHandler_)
    {
      auto self = shared_from_this();
      openHandler_(self);
    }
  }
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleHandshake: " << error.message();
    close();
  }
}

void ClientHTTPSession::asyncRead(const std::shared_ptr<std::string>& message)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  socket_.async_read_some(
    boost::asio::buffer(*message),
    boost::asio::bind_executor(socketStrand_, boost::bind(&ClientHTTPSession::handleRead, shared_from_this(), message, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)));
}

void ClientHTTPSession::handleRead(const std::shared_ptr<std::string>& message, const boost::system::error_code& error, std::size_t bytes_transferred)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  if (!error)
  {
    if (bytes_transferred != 0)
    {
      message->resize(bytes_transferred);
      handleMessage(message);
      readNext();
    }
    else
    {
      LOG_TRACE << "handleReadHeader: verify failed!" << std::endl;
      close();
    }
  }
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleReadHeader: " << error.message() << std::endl;
    close();
  }
}

void Net::ClientHTTPSession::send(const std::shared_ptr<std::string>& message)
{
  std::lock_guard<std::recursive_mutex> lock(queueMutex_);

  bool writeInProgress = !writeQueue_.empty();

  writeQueue_.push_back(message);

  if (!writeInProgress)
  {
    writeNext();
  }
}

void ClientHTTPSession::writeNext()
{
  if (!writeQueue_.empty())
  {
    auto currentMessage = writeQueue_.front();
    boost::asio::post(socketStrand_, boost::bind(&ClientHTTPSession::asyncWrite, shared_from_this(), currentMessage));
  }
}

void ClientHTTPSession::asyncWrite(const std::shared_ptr<std::string>& message)
{
  boost::asio::async_write(socket_, boost::asio::buffer(*message),
    boost::asio::transfer_at_least(message->size()),
    boost::asio::bind_executor(socketStrand_, boost::bind(&ClientHTTPSession::handleWrite, shared_from_this(), message, boost::asio::placeholders::error)));
}

void ClientHTTPSession::handleWrite(const std::shared_ptr<std::string>& message, const boost::system::error_code& error)
{
  {
    std::lock_guard<std::recursive_mutex> lock(queueMutex_);

    if (!writeQueue_.empty())
    {
      writeQueue_.pop_front();
    }

    if (!error)
    {
      writeNext();
    }
    else if (error != boost::asio::error::operation_aborted)
    {
      LOG_TRACE << "close from handleWrite: " << error.message() << std::endl;
      close();
    }
  }
}

void Net::ClientHTTPSession::handleMessage(const std::shared_ptr<std::string>& message)
{
  if (receiveHandler_ && isConnected_)
  {
    boost::asio::post(readStrand_, boost::bind(receiveHandler_, message, shared_from_this()));
  }
}
