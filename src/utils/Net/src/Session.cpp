#include "Session.hpp"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/current_function.hpp>
#include <openssl/ssl3.h>

#include <Buffer.h>

#include "Message.hpp"
#include "MetaMessage.hpp"

#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message()

using namespace Net;

Session::Session(boost::asio::io_context& ioService, boost::asio::ssl::context& context)
  : socket_(ioService, context)
  , socketStrand_(boost::asio::make_strand(ioService))
  , readStrand_(boost::asio::make_strand(ioService))
  , readTimer_(ioService)
  , readTimerStrand_(boost::asio::make_strand(ioService))
  , randomNumberGenerator_(PingTimeOut, PingTimeOut * 0.25)
{
  LOG_TRACE << "session created" << std::endl;

  isConnected_ = false;
}

Session::~Session()
{
  LOG_TRACE << "session deleted" << std::endl;
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

void Session::open(const boost::asio::ssl::stream_base::handshake_type & handshakeType)
{
  if (isConnected_)
  {
    LOG_TRACE << "open: already opened" << std::endl;
    return;
  }

  configureSocket();

  isConnected_ = true;

  // auto self = shared_from_this();
  // boost::asio::post(readTimerStrand_, [this, self]()
  // {
  //   lastTimeRead_ = std::chrono::system_clock::now();
  // });

  auto self = shared_from_this();
  boost::asio::post(socketStrand_, [this, self, handshakeType]()
  {
    asyncHandshake(handshakeType);
  });

  // readNext();

  // if (openHandler_)
  // {
  //   openHandler_(self);
  // }
}

void Session::cancelAllPendingOperations()
{
  auto self = shared_from_this();
  boost::asio::post(socketStrand_, [this, self]()
  {
    boost::system::error_code error;
    socket().cancel(error);

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

boost::asio::ip::tcp::socket& Session::socket()
{
  return socket_.next_layer();
}

void Session::configureSocket()
{
  boost::system::error_code error;

  {
    //boost::asio::ip::tcp::socket::linger opt(false, 0);
    socket().set_option(boost::asio::ip::tcp::no_delay(true));
    //socket().set_option(opt, error);

    LOG_TRACE_IF_ERROR(error, "option linger setted");
  }

  // {
  //   auto cb = [](bool preverified, boost::asio::ssl::verify_context& ctx) {
  //       char subject_name[256];
  //       X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
  //       X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

  //       std::cout << "SSL Verify: " << subject_name << "\n";

  //       return preverified;
  //   };
  //   socket_.set_verify_callback(cb);
  // }
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

void Session::asyncHandshake(const boost::asio::ssl::stream_base::handshake_type& handshakeType)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  socket_.async_handshake(handshakeType,
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleHandshake, shared_from_this(), boost::asio::placeholders::error)));
}

void Session::handleHandshake(const boost::system::error_code& error)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  isConnected_ = true;

  if (!error)
  {
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
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleHandshake: " << error.message();
    close();
  }
}

void Session::asyncReadHeader(const MetaMessagePtr& message)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  boost::asio::async_read(socket_,
    boost::asio::buffer(reinterpret_cast<void*>(&(message->header())), MetaMessage::HeaderSize),
    boost::asio::transfer_at_least(MetaMessage::HeaderSize),
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleReadHeader, shared_from_this(), message, boost::asio::placeholders::error)));

  restartReadTimer();
}

void Session::handleReadHeader(const MetaMessagePtr& message, const boost::system::error_code& error)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

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
        LOG_TRACE << "empty message!" << std::endl;
        onMessageReceived(message);
        readNext();
      }
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

void Session::asyncReadBody(const MetaMessagePtr& message)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

  auto& payload = message->payload();

  boost::asio::async_read(socket_,
    boost::asio::buffer(reinterpret_cast<void*>(payload->data()), payload->size()),
    boost::asio::transfer_at_least(payload->size()),
    boost::asio::bind_executor(socketStrand_, boost::bind(&Session::handleReadBody, shared_from_this(), message, boost::asio::placeholders::error)));

  restartReadTimer();
}

void Session::handleReadBody(const MetaMessagePtr& message, const boost::system::error_code& error)
{
  LOG_TRACE << BOOST_CURRENT_FUNCTION << std::endl;

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
      LOG_TRACE << "handleReadBody: verify failed!" << std::endl;
      close();
    }
  }
  else if (error != boost::asio::error::operation_aborted)
  {
    LOG_TRACE << "close from handleReadBody: " << error.message() << std::endl;
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
      LOG_TRACE << "close from handleWrite: " << error.message() << std::endl;
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
      LOG_TRACE << "handling message!\n" << std::endl;
      handleMessage(message);
    }
  }
  else
  {
    LOG_ERROR << "received message with invalid type! type = " << type << std::endl;
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
      LOG_TRACE << "cancelReadTimer: " << error.message() << std::endl;
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
      LOG_TRACE << "close from handleReadTimeOut: connection lost" << std::endl;

      LOG_TRACE << "timeout = " << timeout << std::endl;
      close();
    }
    else
    {
      ping();

      restartReadTimer();
    }
  });
}
