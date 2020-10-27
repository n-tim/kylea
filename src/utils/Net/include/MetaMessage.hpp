#ifndef NET_META_MESSAGE_H
#define NET_META_MESSAGE_H

#include "Message.hpp"

namespace Net
{
  class MetaMessage: public Message
  {
  public:
    void fitToSize();
    bool isEmpty() const;
    bool verifyHeader() const;
    bool verifyBody() const;
  };
}

#endif // !NET_META_MESSAGE_H