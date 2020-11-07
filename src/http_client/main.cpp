#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <sstream>

#include <boost/asio.hpp>

#include <ThreadPool.h>

#include <ClientHTTPSession.hpp>
#include <boost/asio/ssl.hpp>

using namespace Net;

#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message() << std::endl

bool exceptionHandle(const boost::exception_ptr&, const std::string&)
{
  return false;
}

int main(int argc, char *argv[])
{
  try
  {
    // if (argc < 3)
    // {
    //   std::cerr << "Usage: " << argv[0] << " address " << " port " << std::endl;
    //   return 1;
    // }

    // std::string ip(argv[1]);
    // int port = std::stoi(argv[2]);

    // std::cerr << "args: " << ip << ":" << port << std::endl;

    std::size_t cpuCount = 4;//(!(boost::thread::hardware_concurrency()) ? 2 : boost::thread::hardware_concurrency() * 2);
    boost::asio::io_context ioService(cpuCount);

    std::srand(std::time(0));

    ThreadPool threadPool(ioService, cpuCount);
    threadPool.setExceptionHandler(boost::bind(&exceptionHandle, _1, _2));

    boost::asio::signal_set signas(ioService, SIGINT, SIGTERM);
    signas.async_wait([](const boost::system::error_code& error, int signal_number)
    {
      std::cerr << "signal: " << signal_number << ", msg: " << error.message() << std::endl;

      exit(0);
    });

    {
      using boost::asio::ssl::context;
      boost::system::error_code error;
      boost::asio::ssl::context sslContext_(boost::asio::ssl::context::tlsv12);

      auto cb = [](bool preverified, boost::asio::ssl::verify_context& ctx) {
          char subject_name[256];
          X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
          X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

          std::cout << "SSL Verify: " << subject_name << "\n";

          return true;
      };
      sslContext_.set_default_verify_paths();
      sslContext_.set_verify_callback(cb);
      sslContext_.set_verify_mode(context::verify_none, error);
      if (error)
      {
        LOG_ERROR << "set_verify_mode: " << error.message();
      }

      using boost::asio::ip::tcp;
      using boost::asio::ip::address;

      std::string server = "google.com";

      tcp::resolver resolver(ioService);

      resolver.async_resolve(server, "https", [&ioService, &sslContext_, server](const boost::system::error_code& error,
                        tcp::resolver::results_type results){
        if (!error)
        {
          {

            //LOG_DEBUG << "connecting to " << *endpoint_iterator;

            auto session = ClientHTTPSession::create(ioService, sslContext_);

            auto openedHandle = [server](const ClientHTTPSessionPtr& session)
            {
              LOG_TRACE << "handleSessionOpened:Connected";

              std::stringstream ss;
              ss << "GET /" << "\r\n"
              << "Host: " << server << "\r\n"
              << "Accept: */*\r\n"
              << "Connection: close"
              << "\r\n\r\n";

              auto payload = std::make_shared<std::string>(ss.str());
              session->send(payload);
            };

            auto closedHandle = [](const ClientHTTPSessionPtr&)
            {
              LOG_TRACE << "handleSessionOpened:Closed";
            };

            auto receivedHandle = [](const std::shared_ptr<std::string>& message, const ClientHTTPSessionPtr& session)
            {
              std::cout << *message;
              if (message->empty())
              {
                session->drop();
              }
            };

            session->configure(openedHandle, closedHandle, receivedHandle);
            LOG_TRACE << "async connect" << std::endl;
            boost::asio::async_connect(session->socket(), results, [session](const boost::system::error_code& error, const tcp::endpoint&)
            {
              LOG_TRACE_IF_ERROR(error, "async_connect handle");
              session->open();
            });
          }
        }
        else
        {
          LOG_ERROR << "Error resolve: " << error.message() << "\n";
        }

      });
    }
    LOG_TRACE << "hi there!" << std::endl;
    while(true)
    {

    }

    LOG_TRACE << "bye there!" << std::endl;

    threadPool.stop();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected error: " << e.what();
  }
  catch (...)
  {
    std::cerr << "Unknown exception";
  }
}
