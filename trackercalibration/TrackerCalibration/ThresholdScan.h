#ifndef __ThresholdScan_h__
#define __ThresholdScan_h__

#include "ITest.h"
#include "Chip.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRBEventDecoder.h"

namespace TrackerCalib {
  
  /**
   *  @brief Class implementing a simple threshold scan.
   */  
  class ThresholdScan : public ITest {

  public:
    /** \brief Default constructor */
    ThresholdScan();

    /** \brief Constructor with parameter
     *
     * Threshold scan. WRITE MEEE
     * 
     * @param[in] ntrig    Number of triggers at each scan point
     * @param[in] charge   Input charge (fC)
     * @param[in] autostop Stop scan automatically ? 
     * @param[in] mode     Compression criteria (ABCD_ReadoutMode enum defined in Chip.h) 
     * @param[in] edge     Edge detection mode ? 
     * @param[in] calLoop  send (Pulse+delay+L1A) in calLoop mode ? 
     */
    ThresholdScan(int ntrig,
		  float charge,
		  bool autoStop=true,
		  ABCD_ReadoutMode mode=ABCD_ReadoutMode::LEVEL,
		  bool edge=false);

    /** clone method */
    virtual ThresholdScan* clone() const { return new ThresholdScan(*this); }

    /** Destructor */
    ~ThresholdScan();

    /** clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object (that should be needed). */
    void clear();

    /** \brief Set number of triggers */
    void setNtrig(int ntrig) { m_ntrig = ntrig; }

    /** \brief Set start threshold of scan (DAC units) */
    void setStart(unsigned int start) { m_start = start; } 

    /** \brief Set end threshold of scan (DAC units) */
    void setStop(unsigned int stop) { m_stop = stop; } 

    /** \brief Set increasing step in threshold (DAC units) */
    void setStep(unsigned int step) { m_step = step; } 

    /** \brief Set input charge [fC] */
    void setCharge(float charge);

    /** \brief Set autoStop function */
    void setAutoStop(bool autoStop) { m_autoStop = autoStop; }

    /** \brief Set readout mode (compression critera) */
    void setReadoutMode(ABCD_ReadoutMode rmode) { m_rmode = rmode; }

    /** \brief Set edge detect function */
    void setEdge(bool edge) { m_edge = edge; }

    /** ------------------------------------------------------------
	Set functions for various parameters that are protected
	members of parent class ITest
	----------------------------------------------------------- */
    
    /** \brief Set module global mask. 
	m_globalMask is protected member of parent class ITest. */
    void setGlobalMask(uint8_t globalMask) { m_globalMask = globalMask; }
    
    /** \brief Set calLoop 
	m_calLoop is protected member of parent class ITest. */
    void setCalLoop(bool calLoop) { m_calLoop = calLoop; }
    
    /** \brief Set LoadTrim. 
	m_loadTrim is protected member of parent class ITest. */
    void setLoadTrim(bool loadTrim) { m_loadTrim = loadTrim; }
    
    /** \brief Set SaveDaq. 
	m_saveDaq is protected member of parent class ITest. */
    void setSaveDaq(bool saveDaq) { m_saveDaq = saveDaq; }
    
    /** \brief Set verbosity level. 
	m_printLevel is protected member of parent class ITest. */
    void setPrintLevel(int printLevel) { m_printLevel = printLevel; }
    
    /** \brief Set output directory. Call parent class method since 
	we need to create the output directory in case it does not exists. */
    void setOutputDir(std::string outDir) { ITest::setOutputDir(outDir); }

    /** Set runNumber 
	m_runNumber is protected member of parent class ITest.*/
    void setRunNumber(int runNumber) { m_runNumber=runNumber; }
    
    /** run function. Internally, the private functions 
	initialize, execute and finalize are called. */
    virtual int run(FASER::TRBAccess *trb, 
		    std::vector<Module*> &modList);

    /** Function to fit scurves.
     *
     * @param[in] imodule  Module number [0;7]
     * @param[in] ilink    Link number [0;1]
     * @param[in] ichip    Chip number within link [0;5]
     * @param[in] istrip   Channel number within chip [0;127]
     */
    void fitScurves(int imodule, 
		    int ilink, 
		    int ichip, 
		    int istrip);

    // FIX ME !!
    float vt50(int imodule,
	       int ilink, 
	       int ichip,
	       int istrip);

    // FIX ME !!
    float evt50(int imodule,
	       int ilink, 
	       int ichip,
	       int istrip);

    // FIX ME !!
    float sigma(int imodule,
		int ilink, 
		int ichip,
		int istrip);
    
    /** print function. */
    virtual const std::string print(int indent=0);

    /** returns output ROOT filename. */
    std::string outputFile() const { return m_outFilename; }

  private:
    int initialize(FASER::TRBAccess*, 
		   std::vector<Module*> &modList);
    
    int execute(FASER::TRBAccess*, 
		std::vector<Module*> &modList);
    
    int finalize(FASER::TRBAccess*, 
		 std::vector<Module*> &modList);    

    void setStartThreshold();    

  private:
    unsigned int m_ntrig; /// number of L1A triggers to be sent at each scan-step
    float m_charge; /// input calibration charge [fC]
    bool m_autoStop; /// automatically stop scan ?
    ABCD_ReadoutMode m_rmode;
    bool m_edge; /// edge detect mode
    
    unsigned int m_start; /// start threshold [DAC counts]
    unsigned int m_stop; /// end threshold [DAC counts]
    unsigned int m_step; /// step for threshold loop [DAC counts]

    int m_nullOccCnt; /// counter of zero-occupancy occurrences to autostop scan

    std::string m_outFilename; /// output ROOT filename
    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time

    // hits single chip
    int m_hits[MAXTHR][MAXMODS][NLINKS][NCHIPS][NSTRIPS];

    float m_vt50[MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    float m_evt50[MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    float m_sigma[MAXMODS][NLINKS][NCHIPS][NSTRIPS];

  }; // class ThresholdScan
} // namespace TrackerCalib

#endif
