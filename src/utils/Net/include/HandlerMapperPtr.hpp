#ifndef NET_HANDLERMAPPERPTR_HPP
#define NET_HANDLERMAPPERPTR_HPP

#include <memory>

namespace Net
{
  class HandlerMapper;
  using HandlerMapperPtr = std::shared_ptr<HandlerMapper>;
}

#endif // !NET_HANDLERMAPPERPTR_HPP
