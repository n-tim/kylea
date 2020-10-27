#ifndef NET_HANDLERMAPPER_HPP
#define NET_HANDLERMAPPER_HPP

#include <BufferPtr.h>
#include "SessionInterfacePtr.hpp"

namespace Net
{
  class HandlerMapper
  {
  public:
    virtual void onReceived(const BufferPtr& payload, const SessionInterfacePtr& session) = 0;
  };
}

#endif // !NET_HANDLERMAPPER_HPP
