#ifndef __CalibManager_h__
#define __CalibManager_h__

// TrackerReadout includes
#include "TrackerReadout/TRBAccess.h"

#include <string>
#include <vector>

namespace TrackerCalib {

  /// forward includes from within namespace TrackerCalib
  class Module;
  class ITest;
  class RunManager;
  
  /**
   *  @brief Main class to steer the calibration of the tracker.
   */  
  class CalibManager {
    
  public:

    /** Constructor */
    CalibManager();

    /** TBD writ me */
    CalibManager(std::string outBaseDir,
		 std::string jsonInputFile,
		 std::string logfile,
		 std::vector<int> testSequence,
		 unsigned int l1delay,
		 bool emulate,
		 bool calLoop,
		 bool loadTrim,
		 bool noRunNumber,
		 bool usb,
		 std::string ip,
		 bool saveDaq,
		 int printLevel);
    
    /** Destructor. */
    ~CalibManager();

    /** \brief Initialize method.
     *
     * Shows passed options from command line and initializes TRB.
     * \return Returns 1 on success, 0 on failure.
     */
    int init();

    /** \brief Run method.
     *
     * \return Returns 1 on success, 0 on failure.
     */
    int run();

    /** \brief Finalize method.
     *
     * \return Returns 1 on success, 0 on failure.
     */
    int finalize();

    /** \brief Add a test to the list of tests. */
    void addTest(const TrackerCalib::ITest*);

    /** \brief Add a Module to the list of modules. */
    void addModule(const TrackerCalib::Module*);

    /** \brief Set L1Delay emulation */
    void setL1Delay(unsigned int l1delay) { m_l1delay = l1delay; }

    /** \brief Set TRB emulation */
    void setEmulateTRB(bool emulateTRB) { m_emulateTRB = emulateTRB; }

    /** \brief Set IP address (for ethernet communication) */
    void setIP(std::string ip) { m_ip = ip; }
    
    /** \brief Set verbosity level 
     *
     * The verbosity level will be propagated down
     * to the Modules and ITests.
     */
    void setPrintLevel(int printLevel);

    /** \brief Return output directory */
    std::string outBaseDir() const { return m_outBaseDir; }
    
  private:
    /** Logo functions, essentials !! ;-) */
    const std::string Logo();
    const std::string FaserLogo();
    const std::string CalibLogo();

    /** \brief Read json input file.  
     *  
     * This function parses a json input file and populates the list of
     * Module objects to be processed afterwards. The json input file    
     * can be either a single module config file, or a tracking plane 
     * config file containing a list of module configs (with relative or absolute paths).
     */
    int readJson(std::string jsonCfg);

    /** write final json configuration file (after all tests have been run). */
    int writeJson(std::string outDir);

  private:
    FASER::TRBAccess *m_trb;
    RunManager *m_rman; /// RunManager
    std::vector<Module*> m_modList; /// vector of modules
    std::vector<ITest*> m_testList; /// vector of tests
    std::string m_outBaseDir; /// output base directory
    std::string m_jsonInputFile; /// json input configuration file.
    std::string m_logfile; /// output logfile
    unsigned int m_l1delay;
    uint8_t m_globalMask; 
    bool m_emulateTRB; /// emulate TRB
    bool m_calLoop; /// if using calLoop functionality

    /** \brief load trimDac values. 
     * 	Selecting to -not- load the trimDacs can be found useful to 
     * save some time when performing tests / other developments. */
    bool m_loadTrim; 

    /// automatic creation of run-number to be stored in DB
    bool m_doRunNumber;

    bool m_usb; /// usb TRB via USB (via ethernet otherwise)
    std::string m_ip; /// SCIP / DAQIP address (assumed to be the same) for ethernet communication

    bool m_saveDaq; /// enable creation of .daq file from TRBAccess

    int m_printLevel; /// verbosity level
    
  }; // class CalibManager

} // namespace TrackerCalib

#endif
