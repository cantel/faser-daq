#ifndef __Logger_h__
#define __Logger_h__

#include <iostream>
#include <streambuf>
#include <fstream>

namespace TrackerCalib {

  /** 
   * @class Logger
   *    
   * @brief Class to log into two different ostreams, typically std::cout and std::ofstream.
   *
   * The class follows a singleton design pattern to ensure there is only a single instance 
   * in the program. This is required to guarantee constant output to the logfile (ofstream),
   * where different classes (typically test scans derived from the ITest interface) output
   * information as the scan progresses. in the case of using an ofstream output stream, we
   * assume it is opened before creating the class instance and that is closed when all output
   * operations are no lnger needed.
   */
  class Logger {    
   
  public:
    /** Return instance by reference (not pointer to avoid deletion by client) */
    static Logger& instance();

    /** Block copy/move constructors and assignment/move operators  */
    Logger(Logger const&) = delete;
    Logger(Logger const&&) = delete;
    Logger &operator=(Logger const&) = delete;
    Logger &operator=(Logger const&&) = delete;

    /** \brief Initialize ostream pointers.
     *
     * The two pointer data members are correspondingly 
     * assigned to the address of the two ostream function 
     * arguments.	
     */
    void init(std::ostream &o1, std::ostream &o2);

    /** \brief Output a string to a single ostream only. 
     *
     * @param[in] iostr Numerical identifier for selected ostream
     */
    void extra(const std::string str, int iostr=2);

    /** Overload operator << */
    template<typename T> 
      Logger& operator<< (const T& data) {
      *os1 << data;
      *os2 << data;
      return *this;
    }

    /** Non-templated overloaded operator << for manipulators so that std::endl 
	can be used with the templated overloaded operator << of the class instance. */
    Logger &operator <<(std::ostream& (*os)(std::ostream&)){
      *os1 << os;
      *os2 << os;
      return *this;
    }

  private:
    /** Constructor (private). Needed to create instance. */
    Logger() {};
    
    /** Destructor (private), in case of dynamically allocated objects inside class */
    ~Logger(){};

  protected:
    std::ostream *os1; /// First ostream 
    std::ostream *os2; /// Second ostream 

  }; // class Logger

} // namespace TrackerCalib

#endif
