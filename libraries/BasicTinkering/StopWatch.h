#ifndef __BASIC_TINKERING_STOP_WATCH__
#define __BASIC_TINKERING_STOP_WATCH__

/**
 * A simple, milliseconds-accurate, toggleable stop watch
 */
class StopWatch
{
public:
  // Convert elapsed milliseconds to seconds.
  static double toSeconds(unsigned long elapsed_ms)
  {
    return static_cast<double>(elapsed_ms) / 1000.0;
  }

  StopWatch() 
  {
    reset();
  }

  unsigned long elapsed() const 
  {
    if (is_running_)
    {
      const unsigned long e = millis() - start_time_;
      return elapsed_ + e;
    }
    return elapsed_;
  }

  void reset()
  {
    elapsed_ = 0;
    is_running_ = false;
  }

  void start()
  {
    is_running_ = true;
    start_time_ = millis();
  }

  void toggle()
  {
    if (is_running_)
      stop();
    else
      start();
  }

  void stop()
  {
    elapsed_ = elapsed();
    is_running_ = false;
  }

private:
  unsigned long start_time_;
  unsigned long elapsed_;     //< We accumulate elapsed time (needed because of toggle/pause)
  bool is_running_;
};
#endif //__UTILS_STOP_WATCH__
