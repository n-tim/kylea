#include "Session.hpp"

#include <iostream>

#include <boost/bind.hpp>

#include <Buffer.h>

#include "Message.hpp"
#include "MetaMessage.hpp"

#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message()

using namespace Net;

Session::Session(boost::asio::io_context& ioService)
  : socket_(ioService)
  , socketStrand_(boost::asio::make_strand(ioService))
  , readStrand_(boost::asio::make_strand(ioService))
  , readTimer_(ioService)
  , readTimerStrand_(boost::asio::make_strand(ioService))
  , randomNumberGenerator_(PingTimeOut, PingTimeOut * 0.25)
{
  LOG_TRACE << "session created";

  isConnected_ = false;
}

Session::~Session()
{
  LOG_TRACE << "session deleted";
}

void Session::configure(const OpenHandler& openHandler, const CloseHandler& closeHandler, const ReceiveHandler& receiveHandler)
{
  openHandler_ = openHandler;
  closeHandler_ = closeHandler;
  receiveHandler_ = receiveHandler;
}

void Net::Session::send(const BufferPtr& payload)
{
  auto message = Message::create(MessageType::Data, payload);
  send(message);
}

void Session::drop()
{
  close();
}

void Session::open()
{
  if (isConnected_)
  {
    LOG_TRACE << "open: already opened";
    return;
  }

  configureSocket();

  isConnected_ = true;

  auto self = shared_from_this();
  boost::asio::post(readTimerStrand_, [this, self]()
  {
    lastTimeRead_ = std::chrono::system_clock::now();
  });

  readNext();

  if (openHandler_)
  {
    openHandler_(self);
  }
}

void Session::cancelAllPendingOperations()
{
  auto self = shared_from_this();
  boost::asio::post(socketStrand_, [this, self]()
  {
    boost::system::error_code error;
    socket_.cancel(error);

    LOG_TRACE_IF_ERROR(error, "cancelAllPendingOperations");
  });
}

void Session::close()
{
  cancelAllPendingOperations();
  cancelReadTimer();

  auto self = shared_from_this();

  if (!isConnected_)
  {
    LOG_TRACE << "close: already closed";

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
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
    socket_.close(error);

    if (closeHandler_)
    {
      closeHandler_(self);
    }

    LOG_TRACE << "close: socket closed";
  });
}

boost::asio::ip::tcp::socket& Session::socket()
{
  return socket_;
}

void Session::configureSocket()
{
  boost::system::error_code error;

  {
    boost::asio::ip::tcp::socket::linger opt(false, 0);
    socket_.set_option(opt, error);

    LOG_TRACE_IF_ERROR(error, "option linger setted");
  }
}

void Session::ping()
{
  send(Message::create(MessageType::Ping));
}

void Session::pong()
{
  send(Message::create(MessageType::Pong));
}

void Session::readNext()
{
  auto message = std::make_shared<MetaMessage>();
  asyncReadHeader(message);
}

void Session::asyncReadHeader(const MetaMessagePtr& message)
{
  boost::asio::async_read(socket_,
    boost::asio::buffer(reinterpret_cast<void*>(&(message->header())), MetaMessage::HeaderSize),
    boost::asio::transfer_at_least(MetaMessage::HeaderSize),
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleReadHeader, shared_from_this(), message, boost::asio::placeholders::error)));

  restartReadTimer();
}

void Session::handleReadHeader(const MetaMessagePtr& message, const boost::system::error_code& error)
{
  cancelReadTimer();

  if (!error)
  {
    if (message->verifyHeader())
    {
      if (!message->isEmpty())
      {
        message->fitToSize();
        asyncReadBody(message);
      }
      else
      {
        LOG_TRACE << "empty message!";
        onMessageReceived(message);
        readNext();
      }
    }
    else
    {
      LOG_TRACE << "handleReadHeader: verify failed!";
      close();
    }
  }
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleReadHeader: " << error.message();
    close();
  }
}

void Session::asyncReadBody(const MetaMessagePtr& message)
{
  auto& payload = message->payload();

  boost::asio::async_read(socket_,
    boost::asio::buffer(reinterpret_cast<void*>(payload->data()), payload->size()),
    boost::asio::transfer_at_least(payload->size()),
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleReadBody, shared_from_this(), message, boost::asio::placeholders::error)));

  restartReadTimer();
}

void Session::handleReadBody(const MetaMessagePtr& message, const boost::system::error_code& error)
{
  cancelReadTimer();

  if (!error)
  {
    if (message->verifyBody())
    {
      onMessageReceived(message);

      readNext();
    }
    else
    {
      LOG_TRACE << "handleReadBody: verify failed!";
      close();
    }
  }
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleReadBody: " << error.message();
    close();
  }
}

void Net::Session::send(const MessagePtr& message)
{
  std::lock_guard<std::recursive_mutex> lock(queueMutex_);

  bool writeInProgress = !writeQueue_.empty();

  if (writeInProgress && (message->type() == MessageType::Ping || message->type() == MessageType::Pong))
  {
    auto currentMessage = writeQueue_.front();
    writeQueue_.pop_front();

    writeQueue_.push_front(message);

    writeQueue_.push_front(currentMessage);
  }
  else
  {
    writeQueue_.push_back(message);
  }

  if (!writeInProgress)
  {
    writeNext();
  }
}

void Session::writeNext()
{
  if (!writeQueue_.empty())
  {
    auto currentMessage = writeQueue_.front();
    boost::asio::post(socketStrand_, boost::bind(&Session::asyncWrite, shared_from_this(), currentMessage));
  }
}

void Session::asyncWrite(const MessagePtr& message)
{
  std::vector<boost::asio::const_buffer> buffer;

  buffer.push_back(boost::asio::buffer(reinterpret_cast<const void*>(&message->header())
    , Message::HeaderSize));

  buffer.push_back(boost::asio::buffer(reinterpret_cast<const void*>(message->payload()->data())
    , message->payload()->size()));

  boost::asio::async_write(socket_, buffer,
    boost::asio::transfer_at_least(Message::HeaderSize + message->payload()->size()),
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleWrite, shared_from_this(), message, boost::asio::placeholders::error)));
}

void Session::handleWrite(const MessagePtr& message, const boost::system::error_code& error)
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
      LOG_TRACE << "close from handleWrite: " << error.message();
      close();
    }
  }
}

void Session::onMessageReceived(const MetaMessagePtr& message)
{
  auto type = message->type();
  if (MessageType::Invalid < type && type < MessageType::Last)
  {
    if (type == MessageType::Ping)
    {
      pong();
    }
    else if (type != MessageType::Pong)
    {
      LOG_TRACE << "handling message!\n";
      handleMessage(message);
    }
  }
  else
  {
    LOG_ERROR << "received message with invalid type! type = " << type;
  }
}

void Net::Session::handleMessage(const MetaMessagePtr& message)
{
  if (receiveHandler_ && isConnected_)
  {
    boost::asio::post(readStrand_, boost::bind(receiveHandler_, message->payload()
      , std::static_pointer_cast<SessionInterface>(shared_from_this())));
  }
}

void Session::restartReadTimer()
{
  auto self = shared_from_this();
  boost::asio::post(readTimerStrand_, [this, self]()
  {
    readTimer_.expires_from_now(boost::posix_time::milliseconds((int)randomNumberGenerator_.next()));
    readTimer_.async_wait(boost::bind(&Session::handleReadTimeOut, shared_from_this()
      , boost::asio::placeholders::error));
  });
}

void Session::cancelReadTimer()
{
  auto self = shared_from_this();
  boost::asio::post(readTimerStrand_, [this, self]()
  {
    boost::system::error_code error;

    readTimer_.cancel(error);

    if (error)
    {
      LOG_TRACE << "cancelReadTimer: " << error.message();
    }

    lastTimeRead_ = std::chrono::system_clock::now();
  });
}

void Session::handleReadTimeOut(const boost::system::error_code& error)
{
  if (error == boost::asio::error::operation_aborted)
  {
    return;
  }

  auto self = shared_from_this();
  boost::asio::post(readTimerStrand_, [this, self]()
  {
    auto now = std::chrono::system_clock::now();
    auto timeout = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTimeRead_).count();

    if (timeout > MaxTimeOut)
    {
      LOG_TRACE << "close from handleReadTimeOut: connection lost";

      LOG_TRACE << "timeout = " << timeout;
      close();
    }
    else
    {
      ping();

      restartReadTimer();
    }
  });
}
