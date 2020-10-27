#ifndef NET_SESSION_INTERFACE_PTR_H
#define NET_SESSION_INTERFACE_PTR_H

#include <memory>

namespace Net
{
  class SessionInterface;
  using SessionInterfacePtr = std::shared_ptr<SessionInterface>;
}

#endif // !NET_SESSION_INTERFACE_PTR_H
