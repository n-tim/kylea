#ifndef NET_MESSAGE_TYPES_H
#define NET_MESSAGE_TYPES_H

namespace Net
{
  enum MessageType {
    Invalid,
    Data,
    Ping,
    Pong,
    Last
  };
}

#endif // !NET_MESSAGE_TYPES_H
