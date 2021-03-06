#include "Client.hpp"

#include <boost/asio/ssl.hpp>

#include "Session.hpp"

#include <iostream>
#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message()

using namespace Net;

Client::Client()
  : ioService_(Threads)
  , threadPool_(ioService_, Threads)
  , sslContext_(boost::asio::ssl::context::tlsv12)
{
  threadPool_.setExceptionHandler([](const boost::exception_ptr&, const std::string& msg)
  {
    LOG_ERROR << "Exception in client thread pool! \r\n\t message: " << msg;
    return false;
  });
}

Client::~Client()
{}

bool Client::configure(const std::string& ip, int port, int connectTimeOut)
{
  ip_ = ip;
  port_ = port;
  connectTimeOut_ = connectTimeOut;

  {
    using boost::asio::ssl::context;
    boost::system::error_code error;

    // context::options options = context::default_workarounds
    //   | context::no_sslv2
    //   | context::no_sslv3
    //   | context::no_tlsv1_1;

    // sslContext_.set_options(options, error);
    // if (error)
    // {
    //   LOG_ERROR << "set_options: " << error.message();
    // }

    auto cb = [](bool preverified, boost::asio::ssl::verify_context& ctx) {
        char subject_name[256];
        X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
        X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

        std::cout << "SSL Verify: " << subject_name << "\n";

        return true;
    };
    sslContext_.set_verify_callback(cb);
    sslContext_.set_verify_mode(context::verify_peer, error);
    if (error)
    {
      LOG_ERROR << "set_verify_mode: " << error.message();
    }
  }

  return true;
}

void Client::request(const BufferPtr& payload, const OnSuccess& onSuccess, const onError& onError)
{
  using boost::asio::ip::tcp;
  using boost::asio::ip::address;

  tcp::resolver resolver(ioService_);
  tcp::resolver::query query(ip_, std::to_string(port_));

  boost::system::error_code error;
  auto iter = resolver.resolve(query, error);

  if (error)
  {
    LOG_ERROR << "error creating endpoint: " << error.message();
    LOG_ERROR << "params = " << ip_ << ":" << port_;
    return;
  }

  tcp::endpoint endpoint = *iter;

  LOG_DEBUG << "connecting to " << endpoint;

  auto session = Session::create(ioService_, sslContext_);
  auto timer = Timer::create(ioService_);
  auto connectTimeOut = 3000;

  auto openedHandle = [timer, connectTimeOut, payload](const SessionPtr& session)
  {
    LOG_TRACE << "handleSessionOpened:Connected";
    session->send(payload);
  };

  auto closedHandle = [](const SessionPtr& session)
  {
    LOG_TRACE << "handleSessionOpened:Closed";
  };

  auto receivedHandle = [timer, onSuccess](const BufferPtr& payload, const SessionInterfacePtr& session)
  {
    timer->cancel();

    if (onSuccess)
    {
      onSuccess(payload);
    }

    session->drop();
  };

  session->configure(openedHandle, closedHandle, receivedHandle);
  session->socket().async_connect(endpoint, [session, timer, connectTimeOut, onError](const boost::system::error_code& error)
  {
    timer->start(connectTimeOut, [session, onError](const TimerPtr&)
    {
      LOG_DEBUG << "connect timeout";

      session->cancelAllPendingOperations();
      if (onError)
      {
        onError();
      }
    });
    session->open(boost::asio::ssl::stream_base::client);
  });
}

void Client::kill()
{
  threadPool_.stop();

  LOG_TRACE << "THREAD_POOL STOPPED!";
}