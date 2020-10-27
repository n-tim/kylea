#ifndef NET_SESSION_INTERFACE_H
#define NET_SESSION_INTERFACE_H

#include <BufferPtr.h>
#include "SessionInterfacePtr.hpp"

namespace Net
{
  class SessionInterface
  {
  public:
    using ReceiveHandler = std::function<void(const BufferPtr&, const SessionInterfacePtr&)>;

  public:
    virtual void send(const BufferPtr& payload) = 0;
    virtual void drop() = 0;
  };
}

#endif // !NET_SESSION_INTERFACE_H
