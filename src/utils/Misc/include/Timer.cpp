#include "Timer.h"

#include <boost/asio/post.hpp>
#include <boost/asio/bind_executor.hpp>

Timer::Timer(boost::asio::io_context& ioService)
  : timer_(ioService)
  , strand_(boost::asio::make_strand(ioService))
{}

Timer::~Timer()
{}

TimerPtr Timer::create(boost::asio::io_context& ioService)
{
  return TimerPtr(new Timer(ioService));
}

void Timer::start(int ms, const TimerHandle& timerHandle)
{
  auto self = shared_from_this();
  boost::asio::post(strand_, [=]()
  {
    timer_.expires_from_now(boost::posix_time::milliseconds(ms));
    timer_.async_wait(boost::asio::bind_executor(strand_, [this, self, timerHandle](const boost::system::error_code& error)
    {
      if (error != boost::asio::error::operation_aborted && timerHandle)
      {
        timerHandle(self);
      }
    }));
  });
}

void Timer::shedule(int period, const SheduleHandle& sheduleHandle)
{
  timerHandle_ = [=](const TimerPtr& self)
  {
    if (sheduleHandle && sheduleHandle(self))
    {
      start(period, timerHandle_);
    }
  };

  start(period, timerHandle_);
}

void Timer::shedule(const GetPeriodHandle& getPeriodHandle, const SheduleHandle& sheduleHandle)
{
  timerHandle_ = [=](const TimerPtr& self)
  {
    if (sheduleHandle && getPeriodHandle && sheduleHandle(self))
    {
      start(getPeriodHandle(), timerHandle_);
    }
  };

  if (getPeriodHandle)
  {
    start(getPeriodHandle(), timerHandle_);
  }
}

void Timer::cancel()
{
  auto self = shared_from_this();
  boost::asio::post(strand_, [this, self]()
	{
		timerHandle_.clear();
		timer_.cancel();
	});
}
