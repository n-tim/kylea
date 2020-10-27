#ifndef NET_MESSAGE_PTR_H
#define NET_MESSAGE_PTR_H

#include <memory>

namespace Net
{
  class Message;
  using MessagePtr = std::shared_ptr<const Message>;
}

#endif // !NET_MESSAGE_PTR_H
