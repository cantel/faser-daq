#ifndef __StrobeDelay_h__
#define __StrobeDelay_h__

#include "ITest.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"

namespace TrackerCalib {

  class Module;
  
  /**
   *  @brief Class implementing the StrobeDelay scan.
   */  
  class StrobeDelay : public ITest {
    
  public:
    /** Constructor */
    StrobeDelay();
    
    /** Destructor */
    ~StrobeDelay();

    /** Clone method */
    virtual StrobeDelay* clone() const { return new StrobeDelay(*this); }

    /** \brief Set number of triggers */
    void setNtrig(int ntrig) { m_ntrig = ntrig; }
    
    /** Clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object (that should be needed). */
    void clear();

    /** run function. Internally, the private functions 
	initialize, execute and finalize are called. */
    int run(FASER::TRBAccess *trb, 
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

    void bookTree();

  private:
    unsigned int m_ntrig;/// number of L1A triggers to be sent at each scan-step

    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time

    // hits single chip
    int m_hits[MAXSD][MAXMODS][NLINKS][NCHIPS][NSTRIPS];

  }; // class StrobeDelay

} // namespace TrackerCalib

#endif
