#ifndef BUFFERPROVIDER_H
#define BUFFERPROVIDER_H

#include <mutex>
#include <list>
#include <memory>

#include "BufferPtr.h"
#include "BufferLock.h"
#include "BufferProviderPtr.h"

#include <SharedCreator.hpp>

class BufferProvider : public SharedCreator<BufferProvider>
{
protected:
  BufferProvider(std::size_t maxBufferSize, std::size_t maxBuffersCount);
  virtual ~BufferProvider();

public:
  BufferLockPtr get();
  void reset();

private:
  using Buffers = std::list<BufferPtr>;
  Buffers buffers_;

  std::size_t maxBufferSize_;
  std::size_t maxBuffersCount_;

  std::mutex mutex_;
};

#endif // BUFFERPROVIDER_H
