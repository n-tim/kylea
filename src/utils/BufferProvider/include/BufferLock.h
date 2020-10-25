#ifndef BUFFERLOCK_H
#define BUFFERLOCK_H

#include "Buffer.h"
#include "BufferLockPtr.h"

class BufferLock
{
public:
  BufferLock(const BufferPtr& buffer)
    : buffer_(buffer)
  {
    if (buffer_)
    {
      buffer_->take();
    }
  }

  ~BufferLock()
  {
    if (buffer_)
    {
      buffer_->free();
      buffer_->reset();
    }
  }

  bool empty() { return buffer_.get() == nullptr; }

  const uint8_t* data() const
  {
    return buffer_ ? buffer_->data() : nullptr;
  }

  uint8_t* data()
  {
    return buffer_ ? buffer_->data() : nullptr;
  }

  std::size_t size() const
  {
    return buffer_ ? buffer_->size() : 0;
  }

  void resize(const std::size_t size)
  {
    if (buffer_)
    {
      buffer_->resize(size);
    }
  }

private:
  BufferPtr buffer_;
};

#endif
