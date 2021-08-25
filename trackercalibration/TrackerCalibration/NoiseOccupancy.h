#ifndef __NoiseOccupancy_h__
#define __NoiseOccupancy_h__

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
  class NoiseOccupancy : public ITest {

  public:
    /** \brief Default constructor */
    NoiseOccupancy();

    /** clone method */
    virtual NoiseOccupancy* clone() const { return new NoiseOccupancy(*this); }
    
    /** Destructor */
    ~NoiseOccupancy();
    
    /** clear internal data arrays. Function is public in order to be able
	to safely re-use the same class object. */
    void clear();
    
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

    int execute2(FASER::TRBAccess*, 
		std::vector<Module*> &modList);
    
    int finalize(FASER::TRBAccess*, 
		 std::vector<Module*> &modList);    
    
    void decode(std::vector< std::vector<uint32_t> > &trbEventData, 
		int &ithresholdIdx, 
		int &totEvents, 
		bool &stop);  
    
  private:
    int m_minNumHits; /// min number of hits needed before increasing threshold
    unsigned int m_maxNumTrig; /// maximum number of triggers
    double m_targetOcc; /// target occupancy at which the scan is stopped

    /* [200820] Absolute maximum number of triggers to be sent. If this number is reached then 
       the scan is stopped, because most probably there's a noisy strip somewhere preventing the 
       corresponding module occupancy to go down with increased thresholds.  */
    double m_absMaxNL1A; 

    bool m_autoMask; /// flag too control automatic masking of channels appearing during scan with high occupancies

    uint16_t m_calLoopDelay;    
    bool m_dynamicDelay;

    std::string m_outFilename; /// output ROOT filename
    std::string m_postfix; /// postfix to be added to daq and root filenames so that they match in time

    double m_modocc[8]; /// module occupancies for a given threshold

    int m_hits[MAXTHR][MAXMODS][NLINKS][NCHIPS][NSTRIPS];

    float m_triggers[MAXTHR]; /// number of triggers sent at a given threshold point

  }; // class NoiseOccupancy
} // namespace TrackerCalib

#endif
