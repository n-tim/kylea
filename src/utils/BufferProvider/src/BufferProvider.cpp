#include "BufferProvider.h"

#include <functional>
#include <mutex>

#include"Buffer.h"

BufferProvider::BufferProvider(std::size_t maxBufferSize, std::size_t maxBuffersCount)
  : maxBufferSize_(maxBufferSize)
  , maxBuffersCount_(maxBuffersCount)
{}

BufferProvider::~BufferProvider()
{
  buffers_.clear();
}

BufferLockPtr BufferProvider::get()
{
  std::lock_guard<std::mutex> lock(mutex_);
  auto iter = std::find_if_not(std::begin(buffers_), std::end(buffers_)
    , [](const BufferPtr& buffer){ return buffer->isBusy(); });

  BufferPtr buffer;

  if (iter == std::end(buffers_))
  {
    if (buffers_.size() < maxBuffersCount_)
    {
      buffer = Buffer::create(maxBufferSize_);
      if (buffer)
      {
        buffers_.push_back(buffer);
      }
    }
  }
  else
  {
    buffer = *iter;
  }

  if (buffer)
  {
    return BufferLockPtr(new BufferLock(buffer));
  }
  else
  {
    return BufferLockPtr(nullptr);
  }
}

void BufferProvider::reset()
{
  std::lock_guard<std::mutex> lock(mutex_);

  auto newEnd = std::remove_if(std::begin(buffers_), std::end(buffers_)
    , [](BufferPtr& buffer){ return !(buffer->isBusy()); });

  buffers_.erase(newEnd, std::end(buffers_));
}
