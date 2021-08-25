#ifndef __ITest_h__
#define __ITest_h__

#include "Module.h"
#include "Timer.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"

// ROOT includes
#include <TTree.h>

#include <string>
#include <vector>

namespace TrackerCalib {
  
  /**
   * Test type enum.
   */  
  enum class TestType
  {
    L1_DELAY_SCAN=1,
    MASK_SCAN,
    THRESHOLD_SCAN,
    STROBE_DELAY,
    THREE_POINT_GAIN, 
    TRIM_SCAN,
    RESPONSE_CURVE,
    NOISE_OCCUPANCY,
    TRIGGER_BURST,
    UNKNOWN
    }; 
  
  /**
   *  @brief Abstract interface class for an SCT test.
   */  
  class ITest {
    
  public:
    /** Constructor */
    ITest(TestType testType);
    
    /** Destructor */
    virtual ~ITest();
    
    /** 
     * Perform scan. This is a pure virtual function that 
     * must be implemented by any derived class of this class.
     *
     * @param[in] trb Pointer to FASER::TRBAccess class
     * @param[in] modList Reference to vector of modules
     */
    virtual int run(FASER::TRBAccess *trb, std::vector<Module*> &modList) = 0;
    
    /**
     * Pure virtual clone method that must be implemented
     * by any derived class. 
     */
    virtual ITest* clone() const = 0;
    
    /** Set L1 delay */
    void setL1delay(unsigned int l1delay) { m_l1delay = l1delay; }
    
    /** Set module global mask */
    void setGlobalMask(uint8_t globalMask) { m_globalMask = globalMask; }
    
    /** \brief Set TRB emulation */
    void setEmulateTRB(bool emulateTRB) { m_emulateTRB = emulateTRB; }

    /** \brief Set calLoop */
    void setCalLoop(bool calLoop) { m_calLoop = calLoop; }
   
     /** Set loadTrim */
    void setLoadTrim(bool loadTrim) { m_loadTrim = loadTrim; }

    /** Set saveDaq */
    void setSaveDaq(bool saveDaq) { m_saveDaq = saveDaq; }
    
    /** Set output directory */
    void setOutputDir(std::string outDir);
    
    /** Set verbosity level */
    void setPrintLevel(int printLevel) { m_printLevel = printLevel; }

    /** Set runNumber */
    void setRunNumber(int runNumber) { m_runNumber=runNumber; }
    
    /** Function returning the enum TestType */
    TestType testType() const { return m_testType; }
    
    /** \brief print function. could eventually be overriden by derived class 
     * @param[in] indent Indent level of printout */
    virtual const std::string print(int indent=0);
    
    /** function that returns a string associated to a given TestType enum value */
    const std::string testName();
    
    /** shows elapsed time */
    const std::string printElapsed() { return m_timer->printElapsed(); }
    
    /** Get output directory */
    std::string outputDir() const { return m_outDir; }

    /** Initialize TTree to store metadata. Tree will be stored in the
	output ROOT file of this test. Within this function:
	- new pointer is dynamically allocated
	- branches are set
	- variables are initialized
    */
    void initTree();
    
  protected:
    TestType m_testType;/// test type
    Timer *m_timer; /// Timer class
    unsigned int m_l1delay; ///  delay between calibration-pulse and L1A
    uint8_t m_globalMask; /// global module mask
    bool m_emulateTRB; /// emulate TRB flag
    bool m_calLoop; /// if using calLoop functionality
    bool m_loadTrim; /// load trimDac values flag
    bool m_saveDaq; /// enable creation of .daq file from TRBAccess
    std::string m_outDir; /// output directory for this test    
    int m_printLevel; /// verbosity level
    int m_runNumber; /// runNumber

    std::vector<std::vector<std::vector<uint8_t> > > m_errors; // [module][chips][errors]

    // ROOT Tree
    TTree *m_tree;
    unsigned int t_l1delay;/// l1delay
    unsigned int t_ntrig; /// number of triggers
    int t_readoutMode; /// readoutMode
    bool t_edgeMode; /// edge mode
    unsigned int t_calAmp_n; /// number of calibration charges
    float t_calAmp[MAXCHARGES]; /// array of calibration charges [fC]
    unsigned int t_threshold_n; /// number of threshold points
    float t_threshold[MAXTHR]; /// array of thresholds [mV]
    unsigned int t_planeID; /// planeID
    unsigned int t_module_n; /// number of modules tested
    unsigned long t_sn[MAXMODS]; /// array of module serial number
    int t_trbChannel[MAXMODS]; /// array of trbChannels

  }; // class ITest  

} // namespace TrackerCalib

#endif
