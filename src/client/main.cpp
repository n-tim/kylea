#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <boost/asio.hpp>

#include <ThreadPool.h>

#include <Client.hpp>
#include <Buffer.h>

bool exceptionHandle(const boost::exception_ptr&, const std::string&)
{
  return false;
}

BufferPtr createBufferFromString(const std::string& str)
{
  std::cerr << "createBufferFromString: str = " << str << std::endl;

  auto buffer = Buffer::create(str.size());
  memcpy(reinterpret_cast<void*>(buffer->data()), reinterpret_cast<const void*>(str.c_str()), str.size());

  std::cerr << "createBufferFromString: buffer->size() = " << buffer->size() << std::endl;


  {
    std::string str;
    str.resize(buffer->size());
    memcpy(const_cast<char*>(str.c_str()), buffer->data(), buffer->size());
    std::cerr << "checking!" << std::endl;
    std::cerr << str << std::endl;
  }

  return buffer;
}

int main(int argc, char *argv[])
{
  try
  {
    if (argc < 4)
    {
      std::cerr << "Usage: " << argv[0] << " address " << " port " << " rndDisconnectTime" << std::endl;
      return 1;
    }

    std::string ip(argv[1]);
    int port = std::stoi(argv[2]);
    int rndDisconnectTime = std::stoi(argv[3]);

    std::cerr << "args: " << ip << ":" << port;

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

    Net::Client client;

    client.configure(ip, port);

    client.request(createBufferFromString(std::string("hi there!")), [](const BufferPtr& payload)
    {
      std::cerr << "response: payload->size() = " << payload->size() << std::endl;
      std::string str;
      str.resize(payload->size());
      memcpy(const_cast<char*>(str.c_str()), payload->data(), payload->size());
      std::cerr << "it should be bye there!" << std::endl;
      std::cerr << str << std::endl;
    }, []()
    {
      std::cerr << "onError!";
    });


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
