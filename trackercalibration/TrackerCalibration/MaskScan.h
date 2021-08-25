#ifndef __MaskScan_h__
#define __MaskScan_h__

#include "ITest.h"
#include "Chip.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRBEventDecoder.h"

namespace TrackerCalib {
  
  /**
   *  @brief Class implementing a test to find dead and noise channels.
   */  
  class MaskScan : public ITest {

  public:
    /** \brief Default constructor */
    MaskScan();

    /** \brief Constructor with parameter
     *
     * Mask scan. WRITE MEEE
     * 
     * @param[in] ntrig    Number of triggers at each scan point
     */
    MaskScan(int ntrig);

    /** clone method */
    virtual MaskScan* clone() const { return new MaskScan(*this); }

    /** Destructor */
    ~MaskScan();

    /** clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object (that should be needed). */
    void clear();

    /** \brief Set number of triggers */
    void setNtrig(int ntrig) { m_ntrig = ntrig; }

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
    
    /** \brief Set output directory. 
	m_outDir is protected member of parent class ITest. */
    void setOutputDir(std::string outDir) { m_outDir = outDir; }

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

    void bookTree();

  private:
    unsigned int m_ntrig; /// number of L1A triggers to be sent at each scan-step
    std::string m_outFilename; /// output ROOT filename
    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time
    int m_hits[MAXTHR][MAXMODS][NLINKS][NCHIPS][NSTRIPS]; /// hit array

  }; // class MaskScan

} // namespace TrackerCalib

#endif
