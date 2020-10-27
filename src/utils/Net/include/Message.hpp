#ifndef NET_MESSAGE_H
#define NET_MESSAGE_H

#include <stdint.h>

#include <SharedCreator.hpp>
#include <BufferPtr.h>

#include "MessageTypes.hpp"

namespace Net
{
#pragma pack(push, 1)
  struct PayloadInfo
  {
    uint32_t crc;
    uint32_t size;
    uint16_t type;

    PayloadInfo(const MessageType& type)
      : crc(0)
      , size(0)
      , type(type)
    {}

    PayloadInfo()
      : PayloadInfo(MessageType::Invalid)
    {}
  };

  struct Header
  {
    uint16_t crc;
    PayloadInfo pInfo;

    Header(const MessageType& type)
      : crc(0)
      , pInfo(type)
    {}

    Header()
      : crc(0)
      , pInfo()
    {}
  };
#pragma pack(pop)

  class Message : public SharedCreator<Message>
  {
  protected:
    Message();
    virtual ~Message();

    explicit Message(const MessageType& messageType);
    Message(const MessageType& messageType, const BufferPtr& payload);

  public:
    enum {HeaderSize = sizeof(Header)};

    MessageType type() const;

    const BufferPtr& payload() const;
    BufferPtr& payload();
    // TODO: remove from here
    Header& header();
    const Header& header() const;

  private:
    inline void countPayloadCRC();
    inline void countHeaderCRC();

  private:
    Message(const Message&) = delete;
    Message(Message&&) = delete;
    Message& operator= (const Message&) = delete;
    Message& operator= (Message&&) = delete;

  protected:
    Header header_;
    BufferPtr payload_;
  };
}

#endif // !NET_MESSAGE_H
