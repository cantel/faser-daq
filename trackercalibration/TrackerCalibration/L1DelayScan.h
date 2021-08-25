#ifndef __L1Delay_h__
#define __L1Delay_h__

#include "ITest.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"
#include "TrackerReadout/TRBEventDecoder.h"

namespace TrackerCalib {
  
  /**
   *  @brief Class implementing the L1 stream delay scan.
   */  
  class L1DelayScan : public ITest {

  public:
    /** Constructor */
    L1DelayScan();

    /** clone method */
    virtual L1DelayScan* clone() const { return new L1DelayScan(*this); }

    /** Destructor */
    ~L1DelayScan();

    /** clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object (that should be needed). */
    void clear();

    /** run function. Internally, the private functions 
	initialize, execute and finalize are called. */
    virtual int run(FASER::TRBAccess *trb,
		    std::vector<Module*> &modList);

    /** print function. */
    virtual const std::string print(int indent=0);
    
  private:
    int initialize(FASER::TRBAccess*,
		   std::vector<Module*> &modList);
    
    int execute(FASER::TRBAccess*,
		std::vector<Module*> &modList);
    
    int finalize(FASER::TRBAccess*,
		 std::vector<Module*> &modList);
    
  private:
    unsigned int m_ntrig; /// number of L1A triggers to be sent 
    unsigned int m_startDelay; /// first delay in tested range
    unsigned int m_endDelay;/// last delay in tested range
    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time

    int m_hits[MAXDELAYS][MAXMODS][NLINKS][NCHIPS]; /// number of hits
    
  }; // class L1Delay
  
} // namespace TrackerCalib

#endif
