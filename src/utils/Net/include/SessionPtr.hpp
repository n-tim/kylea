#ifndef NET_SESSION_PTR_H
#define NET_SESSION_PTR_H

#include <memory>

namespace Net
{
  class Session;
  using SessionPtr = std::shared_ptr<Session>;
}

#endif // !NET_SESSION_PTR_H
