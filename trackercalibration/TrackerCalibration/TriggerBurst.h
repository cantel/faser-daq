#ifndef __TriggerBurst_h__
#define __TriggerBurst_h__

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
  class TriggerBurst : public ITest {

  public:
    /** \brief Default constructor */
    TriggerBurst();

    /** \brief Constructor with parameter
     *
     * Trigger burst. 
     * 
     * @param[in] burst_type  Type of burst: 0=L1A, 1=(CalPulse+delay+L1A)
     * @param[in] ntrig       Total number of triggers to send
     * @param[in] threshold   Threshold [mV]
     * @param[in] charge      Input charge (fC) - just used in case burst_type=1
     * @param[in] mode        Compression criteria (ABCD_ReadoutMode enum defined in Chip.h) 
     * @param[in] edge        Edge detection mode ? 
     */
    TriggerBurst(int burstType,
		 int ntrig,
		 float threshold, 
		 float charge,
		 ABCD_ReadoutMode mode=ABCD_ReadoutMode::LEVEL,
		 bool edge=false);
    
    /** clone method */
    virtual TriggerBurst* clone() const { return new TriggerBurst(*this); }

    /** Destructor */
    ~TriggerBurst();

    /** clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object (that should be needed). */
    void clear();
    
    /** \brief Set burst type */
    void setBurstType(int burstType) { m_burstType = burstType; }

    /** \brief Set total number of triggers */
    void setNtrig(int ntrig) { m_ntrig = ntrig; }

    /** \brief Set triggers per burst */
    void setNtrigBurst(int ntrig) { m_ntrigBurst = ntrig; }

    /** \brief Set threshold [mV] */
    void setThreshold(float threshold);

    /** \brief Set input charge [fC] */
    void setCharge(float charge) { m_charge = charge; };

    /** \brief Set readout mode (compression critera) */
    void setReadoutMode(ABCD_ReadoutMode rmode) { m_rmode = rmode; }

    /** \brief Set edge detect function */
    void setEdge(bool edge) { m_edge = edge; }

    /** Set functions for various parameters that are protected
	members of parent class ITest*/
    
    /** \brief Set module global mask. 
	m_globalMask is protected member of parent class ITest. */
    void setGlobalMask(uint8_t globalMask) { m_globalMask = globalMask; }
    
    /** \brief Set LoadTrim. 
	m_loadTrim is protected member of parent class ITest. */
    void setLoadTrim(bool loadTrim) { m_loadTrim = loadTrim; }
    
    /** \brief Set verbosity level. 
	m_printLevel is protected member of parent class ITest. */
    void setPrintLevel(int printLevel) { m_printLevel = printLevel; }
    
    /** \brief Set output directory. Call parent class method since 
	we need to create the output directory in case it does not exists. */
    void setOutputDir(std::string outDir) { ITest::setOutputDir(outDir); }
    
    /** run function. Internally, the private functions 
	initialize, execute and finalize are called. */
    virtual int run(FASER::TRBAccess *trb, 
		    std::vector<Module*> &modList);

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

  private:
    int m_burstType; /// type of burst: 0=L1A, 1=(CalPulse+delay+L1A)
    unsigned int m_ntrig; /// total number of L1A triggers to be sent
    unsigned int m_ntrigBurst; /// number of triggers per burst to be sent
    float m_threshold; /// threshold [mV]
    float m_charge; /// input calibration charge [fC]
    ABCD_ReadoutMode m_rmode;
    bool m_edge; /// edge detect mode
    
    std::string m_outFilename; /// output ROOT filename
    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time

    // hits single chip
    int m_hits[MAXMODS][NLINKS][NCHIPS][NSTRIPS];

  }; // class TriggerBurst
} // namespace TrackerCalib

#endif
