#include "Buffer.h"

Buffer::Buffer()
  : Buffer(0)
{}

Buffer::Buffer(std::size_t size)
  : data_(size)
  , size_(size)
  , busy_(false)
{}

Buffer::~Buffer()
{}

const uint8_t* Buffer::data() const
{
  return data_.data();
}

uint8_t* Buffer::data()
{
  return data_.data();
}

std::size_t Buffer::size() const
{
  return size_;
}

void Buffer::resize(std::size_t size)
{
  size_ = size;
  data_.resize(size_);
}

void Buffer::reset()
{
  size_ = 0;
}
