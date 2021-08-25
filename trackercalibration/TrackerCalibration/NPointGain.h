#ifndef __NPointGain_h__
#define __NPointGain_h__

#include "ITest.h"
#include "Constants.h"

// gpiodrivers includes
#include "TrackerReadout/TRBAccess.h"

namespace TrackerCalib {
  
  class Module;
  
  /**
   *  @brief Class implementing the NPointGain scan.
   */  
  class NPointGain : public ITest {
    
  public:
    /** \brief Constructor 
     *
     * @parmp[in] type TestType as defined in ITest.h, should be 
     * either THREE_POINT_GAIN or RESPONSE_CURVE. If none of the two
     * is passed as ar argument, an exception is thrown.
`     */
    NPointGain(TestType type);
    
    /** Destructor */
    ~NPointGain();
    
    /** clone method */
    virtual NPointGain* clone() const { return new NPointGain(*this); }
    
    /** \brief Set number of triggers */
    void setNtrig(int ntrig) { m_ntrig = ntrig; }
    
    /** \brief Get number of charges */
    unsigned int Ncharges() { return m_charges.size(); }

    int run(FASER::TRBAccess *trb,
	    std::vector<Module*> &modList);
    
    /** print function. */
    virtual const std::string print(int indent=0);
    
  private:
    void initArrays();	   
    
    int finalize(FASER::TRBAccess*, 
		 std::vector<Module*> &modList);    

  private:
    unsigned int m_ntrig;/// number of L1A triggers to be sent at each scan-step
    
    std::vector<float> m_charges; /// vector of input charges    
    
    double m_vt50[MAXCHARGES][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    double m_evt50[MAXCHARGES][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    double m_sigma[MAXCHARGES][MAXMODS][NLINKS][NCHIPS][NSTRIPS];
    
  }; // class NPointGain

} // namespace TrackerCalib

#endif
