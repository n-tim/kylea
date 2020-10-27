#include "Message.hpp"

#include <Buffer.h>

using namespace Net;

Message::Message()
  : Message(MessageType::Invalid)
{}

Message::~Message()
{}

Message::Message(const MessageType& messageType)
  : Message(messageType, Buffer::create())
{}

Message::Message(const MessageType& messageType, const BufferPtr& payload)
  : header_(messageType)
  , payload_(payload)
{
  countHeaderCRC();
  countPayloadCRC();
}

MessageType Message::type() const
{
  return static_cast<MessageType>(header_.pInfo.type);
}

const BufferPtr& Message::payload() const
{
  return payload_;
}

BufferPtr& Message::payload()
{
  return payload_;
}

Header& Message::header()
{
  return header_;
}
const Header& Message::header() const
{
  return header_;
}

inline void Message::countPayloadCRC()
{
  header_.pInfo.size = payload_->size();
  header_.pInfo.crc = 0;//crc32(reinterpret_cast<uint8_t*>(payload_.data()), payload_.size());
}

inline void Message::countHeaderCRC()
{
  header_.crc = 0;//crc16(reinterpret_cast<uint8_t*>(&(header_.pInfo)), sizeof(PayloadInfo));
}
