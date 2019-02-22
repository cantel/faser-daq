#ifndef DAQ_UTILITIES_COMMON_HPP_
#define DAQ_UTILITIES_COMMON_HPP_

#include <pthread.h>
#include <time.h>

#include "utilities/Types.hpp"

namespace daq {
namespace utilities {

/*
 * Constants
 * */
class Constant {
public:
  // Constants
  static const u_long Kilo = 1024;
  static const u_long Mega = 1024 * Kilo;
  static const u_long Giga = 1024 * Mega;
};


/* RateLimiter usage:
 *
 *  auto limit = RateLimiter();
 *  timestamp_t period_ts = cfg.period * us;
 *  timestamp_t max_overshoot_ts = cfg.max_overshoot * ms;
 *  timestamp_t duration_ts = cfg.duration * s;
 *
 *  if (now > deadline + max_overshoot_ts) {
 *   deadline = now + period_ts;
 *  } else {
 *    while (now < deadline) {
 *      now = limit.gettime();
 *    }
 *    now = limit.gettime();
 *    deadline += period_ts;
 *  }
 *
 */
class RateLimiter
{
public:
  RateLimiter()
  {
  }
  timestamp_t gettime()
  {
    ::timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return timestamp_t(ts.tv_sec) * s + timestamp_t(ts.tv_nsec) * ns;
  }
};


/*
 * setThreadName
 * */
inline void setThreadName(std::thread& thread, const char* name, uint32_t tid)
{
    char tname[16];
    snprintf(tname, 16, "daq-%s-%d", name, tid);
    auto handle = thread.native_handle();
    pthread_setname_np(handle, tname);
}

/*
 * getTime
 * */
inline timestamp_t getTime()
{
  ::timespec ts;
  ::clock_gettime(CLOCK_MONOTONIC,&ts);
  return timestamp_t(ts.tv_sec) * s + timestamp_t(ts.tv_nsec) * ns;
}

/*
 * getExecutablePath
 * */
inline std::string getExecutablePath()
{
  char exePath[PATH_MAX];
  ssize_t len = ::readlink("/proc/self/exe", exePath, sizeof(exePath));
  if (len == -1 || len == sizeof(exePath))
    len = 0;
  exePath[len] = '\0';
  return std::string(exePath);
}


inline std::string methodName(const std::string& prettyFunction)
{
  size_t colons = prettyFunction.find("::");
  size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
  size_t end = prettyFunction.rfind("(") - begin;
  return prettyFunction.substr(begin,end) + "()";
}

inline std::string className(const std::string& prettyFunction)
{
    size_t colons = prettyFunction.find("::");
    if (colons == std::string::npos)
        return "::";
    size_t begin = prettyFunction.substr(0,colons).rfind(" ") + 1;
    size_t end = colons - begin;

    return prettyFunction.substr(begin,end);
}


}
}


#endif

