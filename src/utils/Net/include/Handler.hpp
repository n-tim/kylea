#ifndef NET_HANDLER_HPP
#define NET_HANDLER_HPP

#include <Utils//SharedCreator/SharedCreator.hpp>

#include <Net/Message/MessagePtr.hpp>
#include <Net/Session/SessionInterfacePtr.hpp>

#include "HandlerPtr.hpp"

namespace Net
{
  class Handler : public SharedCreator<HandlerPtr, Handler>
  {
  public:
    virtual void handle(const MessagePtr&, const SessionInterfacePtr&) {}
    virtual bool canHandle(const MessagePtr&) { return false; }

  };
}


#endif // !NET_HANDLER_HPP
