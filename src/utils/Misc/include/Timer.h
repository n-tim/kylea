#ifndef TIMER_H
#define TIMER_H

#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>

class Timer;
typedef boost::shared_ptr<Timer> TimerPtr;

class Timer : public boost::enable_shared_from_this<Timer>
{
  template<class T>
  friend void boost::checked_delete(T* x) BOOST_NOEXCEPT;

public:
  typedef boost::function<void(const TimerPtr&)> TimerHandle;
  typedef boost::function<bool(const TimerPtr&)> SheduleHandle;
  typedef boost::function<int()> GetPeriodHandle;

protected:
  Timer(boost::asio::io_context& ioService);
  virtual ~Timer();

public:
  static TimerPtr create(boost::asio::io_context& ioService);

  void start(int ms, const TimerHandle& timerHandle);

  void shedule(int period, const SheduleHandle& sheduleHandle);
  void shedule(const GetPeriodHandle& getPeriodHandle, const SheduleHandle& sheduleHandle);

  void cancel();

private:
  boost::asio::deadline_timer timer_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;

  TimerHandle timerHandle_;
};



#endif // ! TIMER_H
