#ifndef RANDOM_NUMBER_GENERATOR_H
#define RANDOM_NUMBER_GENERATOR_H

#include <random>

template <typename T = double>
class RandomNumberGenerator
{
public:
  RandomNumberGenerator(T mean, T sigma)
    : randomDev_()
    , engine_(randomDev_())
    , distribution_(mean, sigma)
  {}

  virtual ~RandomNumberGenerator()
  {}

  inline T next()
  {
    return distribution_(engine_);
  }

private:
  std::random_device randomDev_;
  std::mt19937 engine_;
  std::normal_distribution<T> distribution_;
};

#endif // !RANDOM_NUMBER_GENERATOR_H
