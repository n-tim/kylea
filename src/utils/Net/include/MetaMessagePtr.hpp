#ifndef NET_META_MESSAGE_PTR_H
#define NET_META_MESSAGE_PTR_H

#include <memory>

namespace Net
{
  class MetaMessage;
  using MetaMessagePtr = std::shared_ptr<MetaMessage>;
}

#endif // !NET_META_MESSAGE_PTR_H
