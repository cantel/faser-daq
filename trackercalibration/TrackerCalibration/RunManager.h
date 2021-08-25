#ifndef __RunManager_h__
#define __RunManager_h__

#include "nlohmann/json.hpp"
#include <string>

namespace TrackerCalib {

  /**
   *  @brief Class to interface with run service
   */  
  class RunManager {
    
  public:
    /** Constructor */
    RunManager();

    /** Constructor */
    RunManager(int printLevel);

    /** Destructor */
    ~RunManager();

    /** \brief create new run
     *
     * @param[in] jsonCfg Input json config filename
     * @param[in] jtrbConfig Json object with TRB config info
     * @param[in] jscanConfig Json object with scan info
     * \return Status-code of new run number POST request
     */
    int newRun(std::string jsonCfg, 
	       nlohmann::json &jtrbConfig,
	       nlohmann::json &jscanConfig);
    
    /** \brief set end-time for run
     *
     * @param[in] jdata json object for runinfo
     * \return Status-code of new run number POST request
     */
    int addRunInfo(nlohmann::json &jdata);

    /** \brief Returns current run number. */
    int runNumber() const { return m_runNumber; }

  private:
    int m_runNumber; /// run number
    int m_printLevel; /// verbosity level
    
  }; // class RunManager

} // namespace TrackerCalib

#endif
