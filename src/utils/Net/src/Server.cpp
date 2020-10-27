#include "Server.hpp"

#include "HandlerMapper.hpp"
#include "Session.hpp"


#include <iostream>
#define LOG_TRACE std::cerr
#define LOG_ERROR std::cerr
#define LOG_DEBUG std::cerr
#define LOG_TRACE_IF_ERROR(error, msg) if(error) std::cerr << msg << ": " << error.message()

using namespace Net;

Server::Server(const HandlerMapperCreateFunction& createHandlerMapper)
  : ioService_(Threads)
  , threadPool_(ioService_, Threads)
  , acceptor_(ioService_)
  , endpoint_()
  , createHandlerMapper_(createHandlerMapper)
  , sessionManager_(ioService_)
{
  threadPool_.setExceptionHandler([](const boost::exception_ptr&, const std::string& msg)
  {
    LOG_ERROR << "Exception in server thread pool! \r\n\t message: " << msg;
    return false;
  });
}

Server::~Server()
{}

bool Server::start(const std::string& ip, int port)
{
  using boost::asio::ip::tcp;

  if (ip == "")
  {
    endpoint_ = tcp::endpoint(tcp::v4(), port);
  }
  else
  {
    endpoint_ = tcp::endpoint(boost::asio::ip::address::from_string(ip), port);
  }

  acceptor_.open(endpoint_.protocol());
  boost::system::error_code error;

  acceptor_.bind(endpoint_, error);
  if (error)
  {
    return false;
  }

  acceptor_.listen(tcp::acceptor::socket_base::max_connections, error);
  if (error)
  {
    return false;
  }

  LOG_DEBUG << "server started";

  accept();

  return true;
}

bool Server::stop()
{
  bool ok = true;
  boost::system::error_code ec;
  acceptor_.cancel(ec);
  acceptor_.close(ec);
  ok &= !ec;

  LOG_DEBUG << "server stopped";

  sessionManager_.forEach([](const SessionPtr& session)
  {
    session->close();
  });

  threadPool_.stop();

  return ok;
}

void Server::accept()
{
  SessionPtr session = Session::create(ioService_);

  SessionInterface::ReceiveHandler receiveHandler = nullptr;

  if (createHandlerMapper_)
  {
    auto handlerMapper = createHandlerMapper_(this);
    if (handlerMapper)
    {
      receiveHandler = std::bind(&HandlerMapper::onReceived, handlerMapper
        , std::placeholders::_1, std::placeholders::_2);
    }
    else
    {
      LOG_ERROR << "got empty handler mapper";
    }
  }
  else
  {
    LOG_ERROR << "handler mapper creation function not set!";
  }

  session->configure(std::bind(&SessionManager::join, &sessionManager_, std::placeholders::_1)
    , std::bind(&SessionManager::leave, &sessionManager_, std::placeholders::_1)
    , receiveHandler);

  acceptor_.async_accept(session->socket()
    , boost::bind(&Server::handleAccept, this, session, boost::asio::placeholders::error));
}

void Server::handleAccept(const SessionPtr& session, const boost::system::error_code& error)
{
  LOG_TRACE_IF_ERROR(error, "accepted");

  if (error != boost::asio::error::operation_aborted)
  {
    if (!error)
    {
      session->open();
    }

    accept();
  }
}
