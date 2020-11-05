#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <boost/asio.hpp>

#include <ThreadPool.h>

#include <Client.hpp>
#include <Buffer.h>
#include <auth.pb.h>

bool exceptionHandle(const boost::exception_ptr&, const std::string&)
{
  return false;
}

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

int main(int argc, char *argv[])
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;

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

    std::cerr << "args: " << ip << ":" << port << std::endl;

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

    std::string input;
    std::cout << "enter message: login \n password" << std::endl;
    while (true)
    {
      kylea::LoginRequest login;
      std::cin >> *login.mutable_login() >> *login.mutable_password();
      std::cerr << "sending request" << login.DebugString() << std::endl;

      auto buffer = Buffer::create(login.ByteSizeLong());
      login.SerializeToArray(buffer->data(), buffer->size());

      client.request(buffer, [](const BufferPtr& payload)
      {
        kylea::LoginResponse response;
        response.ParseFromArray(payload->data(), payload->size());
        std::cerr << response.DebugString() << std::endl;
      }, []()
      {
        std::cerr << "onError!";
      });
    }

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
