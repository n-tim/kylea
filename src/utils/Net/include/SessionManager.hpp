#ifndef NET_SESSION_MANAGER_H
#define NET_SESSION_MANAGER_H

#include <vector>

#include <boost/asio/io_service.hpp>
#include <boost/asio/strand.hpp>

#include "SessionPtr.hpp"

namespace Net
{
  class SessionManager
  {
  public:
    SessionManager(boost::asio::io_context& ioService);
    virtual ~SessionManager();

    SessionManager(SessionManager&&) = default;
    SessionManager& operator=(SessionManager&&) = default;

    void join(const SessionPtr& session);
    void leave(const SessionPtr& session);

    using Function = std::function<void(const SessionPtr& session)>;
    void forEach(const Function& function);

  private:
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

  private:
    boost::asio::strand<boost::asio::io_context::executor_type> sessionStrand_;

    std::vector<SessionPtr> sessions_;
  };
}

#endif // !NET_SESSION_MANAGER_H
