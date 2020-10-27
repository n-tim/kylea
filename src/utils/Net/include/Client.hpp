#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <memory>
#include <functional>

#include <BufferPtr.h>
#include <Timer.h>
#include <ThreadPool.h>

namespace Net
{
  class Client
  {
    enum { DefaultTimeout = 2 * 1000, Threads = 3 };

  public:
    Client();
    virtual ~Client();

    bool configure(const std::string& ip, int port, int connectTimeOut = DefaultTimeout);

    using OnSuccess = std::function<void(const BufferPtr&)>;
    using onError = std::function<void()>;
    void request(const BufferPtr& payload, const OnSuccess& onSuccess, const onError& onError);
    void kill();

  private:
    boost::asio::io_context ioService_;
    ThreadPool threadPool_;

    std::string ip_;
    int port_;
    int connectTimeOut_;
  };
}

#endif // !NET_CLIENT_H
