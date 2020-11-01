#include <iostream>
#include <cstdlib>
#include <cstring>

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>

#include "ThreadPool.h"

#include <HandlerMapper.hpp>
#include <Server.hpp>
#include <Buffer.h>
#include <SessionInterface.hpp>

BufferPtr stringToBuffer(const std::string& str)
{
  auto buffer = Buffer::create(str.size());
  memcpy(reinterpret_cast<void*>(buffer->data()), reinterpret_cast<const void*>(str.c_str()), str.size());

  return buffer;
}

std::string bufferToString(const BufferPtr& buffer)
{
  std::string str;
  str.resize(buffer->size());
  memcpy(const_cast<char*>(str.c_str()), buffer->data(), buffer->size());

  return str;
}

bool exceptionHandle(const boost::exception_ptr&, const std::string& error_message)
{
  std::cerr << "exceptionHandle: " << error_message << std::endl;
  return false;
}

int main(int argc, char *argv[])
{
  try
  {
    if (argc < 3)
    {
      std::cerr << "Usage: " << argv[0] << " address " << " port " << std::endl;
      return 1;
    }

    std::cerr << "application started";

    std::string ip(argv[1]);
    int port = std::stoi(argv[2]);

    std::cerr << "args: " << ip << ":" << port;

    std::size_t cpuCount = 4;
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

    Net::Server server([](Net::Server* server)
    {
      class BasicHandlerMapper : public Net::HandlerMapper
      {
      public:
        BasicHandlerMapper(Net::Server*)
          : Net::HandlerMapper()
        {}

        virtual void onReceived(const BufferPtr& payload, const Net::SessionInterfacePtr& session) override
        {
          session->send(stringToBuffer(bufferToString(payload) + "_answer"));
        }
      };

      std::cerr << "creating handler\n";
      return std::make_shared<BasicHandlerMapper>(server);
    });

    server.start(ip, port);

    threadPool.stop();

    std::cerr << "application stopped";

    return 0;
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
