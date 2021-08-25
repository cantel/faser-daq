#ifndef __TrimScan_h__
#define __TrimScan_h__

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
  class TrimScan : public ITest {

  public:
    /** \brief Default constructor */
    TrimScan();

    /** \brief Constructor with parameter
     *
     * TrimScan scan. WRITE MEEE
     * 
     * @param[in] ntrig Number of triggers at each scan point
     */
    TrimScan(unsigned int ntrig);
    
    /** clone method */
    virtual TrimScan* clone() const { return new TrimScan(*this); }
    
    /** Destructor */
    ~TrimScan();
    
    /** run function. Internally, the private functions 
	initialize, execute and finalize are called. */
    virtual int run(FASER::TRBAccess *trb, 
		    std::vector<Module*> &modList);
    
    /** print function. */
    virtual const std::string print(int indent=0);
    
  private:
    void clear();

    void initArrays();
    
    int initialize(FASER::TRBAccess*, 
		   std::vector<Module*> &modList);
    
    int execute(FASER::TRBAccess*, 
		std::vector<Module*> &modList);
    
    int finalize(FASER::TRBAccess*, 
    		 std::vector<Module*> &modList);    

  private:
    unsigned int m_ntrig; /// number of L1A triggers to be sent at each scan-step
    
    std::vector<int> m_vtrimdac[MAXTRIMRANGE]; /// vector of trimDac values for every range
    std::vector<std::string> m_filesSummary[MAXTRIMRANGE];/// outputfilenames for each trimDac for every range

    float m_vt50[MAXTRIMRANGE][MAXTRIMDAC][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    float m_evt50[MAXTRIMRANGE][MAXTRIMDAC][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    float m_sigma[MAXTRIMRANGE][MAXTRIMDAC][MAXMODS][NLINKS][NCHIPS][NSTRIPS];

  }; // class TrimScan

} // namespace TrackerCalib

#endif
