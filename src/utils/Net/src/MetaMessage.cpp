#include "MetaMessage.hpp"

#include <assert.h>

#include <Buffer.h>

//#include <Utils/crc/crc16.hpp>
//#include <Utils/crc/crc32.hpp>

using namespace Net;

void MetaMessage::fitToSize()
{
  payload_->resize(header_.pInfo.size);
}

bool MetaMessage::isEmpty() const
{
  return header_.pInfo.size == 0;
}

bool MetaMessage::verifyHeader() const
{
  bool isCrcCorrect = header_.crc == 0;//crc16(reinterpret_cast<const uint8_t*>(&(header_.pInfo)), sizeof(PayloadInfo));

  if (isCrcCorrect)
  {
    bool isPayloadInfoCorrect = true;
    //isPayloadInfoCorrect = isPayloadInfoCorrect && (header_.pInfo.partCount <= MaxPartCount);
    //isPayloadInfoCorrect = isPayloadInfoCorrect && (header_.pInfo.size <= MaxSize);
    return isPayloadInfoCorrect;
  }

  return false;
}

bool MetaMessage::verifyBody() const
{
  return header_.pInfo.crc == 0;//crc32(reinterpret_cast<const uint8_t*>(payload_.data()), payload_.size());
}