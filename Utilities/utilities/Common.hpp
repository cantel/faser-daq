#ifndef UTILITIES_HH_
#define UTILITIES_HH_

#include <pthread.h>

namespace daq {
namespace utilities {

inline void setThreadName(std::thread& thread, const char* name, uint32_t tid)
{
    char tname[16];
    snprintf(tname, 16, "daq-%s-%d", name, tid);
    auto handle = thread.native_handle();
    pthread_setname_np(handle, tname);
}

}
}

#endif

