#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>
#include <vector>
#include <memory>
#include <new>

#include <SharedCreator.hpp>
#include "BufferPtr.h"

class Buffer : public SharedCreator<Buffer>
{
  friend class BufferLock;

public:
  const uint8_t* data() const;
  uint8_t* data();
  std::size_t size() const;

  void resize(std::size_t size);

  void reset();

  inline bool isBusy() { return busy_; }
  inline void take() { busy_ = true; }
  inline void free() { busy_ = false; }

protected:
  Buffer(std::size_t maxSize);
  virtual ~Buffer();

  Buffer(const Buffer& buffer) = delete;
  Buffer& operator=(const Buffer& buffer) = delete;

private:
  using RawBuffer = std::vector<uint8_t>;
  RawBuffer data_;
  std::size_t size_;

  bool busy_;
};

#endif // BUFFER_H
