#ifndef NET_HANDLERPTR_HPP
#define NET_HANDLERPTR_HPP

#include <memory>

namespace Net
{
  class Handler;
  using HandlerPtr = std::shared_ptr<Handler>;
}

#endif // !NET_HANDLERPTR_HPP
