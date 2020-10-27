#include "SessionManager.hpp"

#include "Session.hpp"

using namespace Net;

SessionManager::SessionManager(boost::asio::io_context& ioService)
  : sessionStrand_(boost::asio::make_strand(ioService))
{
  sessions_.clear();
}

SessionManager::~SessionManager()
{}

void SessionManager::join(const SessionPtr & session)
{
  boost::asio::post(sessionStrand_, [=]()
  {
    sessions_.push_back(session);
  });
}

void SessionManager::leave(const SessionPtr & session)
{
  boost::asio::post(sessionStrand_, [=]()
  {
    auto newEnd = std::remove_if(std::begin(sessions_), std::end(sessions_)
      , [session](const SessionPtr& ss) { return ss == session; });

    sessions_.erase(newEnd, std::end(sessions_));
  });
}

void SessionManager::forEach(const Function& function)
{
  boost::asio::post(sessionStrand_, [=]()
  {
    std::for_each(std::begin(sessions_), std::end(sessions_), function);
  });
}
