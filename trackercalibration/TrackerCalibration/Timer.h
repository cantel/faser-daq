#ifndef __Timer_h__
#define __Timer_h__

#include <chrono>
#include <string>

namespace TrackerCalib {
  
  /**
   *  @brief Simple class to deal with elapsed times. 
   *
   *  This class uses steady_clock (monotonic clock that will never be adjusted).
   */  
  class Timer {
    
  public:
    /** \brief Constructor */
    Timer();
    
    /** \brief Destructor, currently empty. */
    ~Timer() {};

    /** \brief Start timer. */
    void start(); 

    /** \brief Stop timer. */
    void stop(); 

    /** \brief Elpased time between start of timer and now. */
    //    std::chrono::seconds elapsed();

    /**
     * Shows elpased time between two intervals. 
     *  - If timer is running: elapsed time between now and the starting time.
     *  - if timer is stopped: elapsed time between stop and start;
     */
    const std::string printElapsed();
    
  private:
    std::chrono::time_point<std::chrono::steady_clock> m_start; /// start time
    std::chrono::time_point<std::chrono::steady_clock> m_stop; /// start time
    bool m_isRunning; /// if timer is running or not

  }; // class Timer  

} // namespace TrackerCalib

#endif
