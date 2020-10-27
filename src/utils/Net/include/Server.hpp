#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <string>

#include <boost/asio.hpp>

#include <ThreadPool.h>

#include "HandlerMapperCreateFunction.hpp"
#include "SessionPtr.hpp"
#include "SessionManager.hpp"

namespace Net
{
  class Server
  {
    enum { Threads = 16 };

  public:
    Server(const HandlerMapperCreateFunction& createHandlerMapper);
    virtual ~Server();

    Server(Server&&) = default;
    Server& operator=(Server&&) = default;

    bool start(const std::string& address, int port);
    bool stop();

  private:
    void accept();
    void handleAccept(const SessionPtr& session, const boost::system::error_code& error);

  private:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

  private:
    boost::asio::io_context ioService_;
    ThreadPool threadPool_;

    boost::asio::ip::tcp::acceptor acceptor_;
    boost::asio::ip::tcp::endpoint endpoint_;

    HandlerMapperCreateFunction createHandlerMapper_;

    SessionManager sessionManager_;
  };
}

#endif // !NET_SERVER_H