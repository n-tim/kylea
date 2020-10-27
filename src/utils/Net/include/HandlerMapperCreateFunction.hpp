#ifndef NET_HANDLERMAPPERCREATEFUNCTION_HPP
#define NET_HANDLERMAPPERCREATEFUNCTION_HPP

#include <functional>

#include "HandlerMapperPtr.hpp"

namespace Net
{
  class Server;
  using HandlerMapperCreateFunction = std::function<HandlerMapperPtr(Server*)>;
}

#endif // !NET_HANDLERMAPPERCREATEFUNCTION_HPP
