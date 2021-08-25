#ifndef __Chip_h__
#define __Chip_h__

#include <string>
#include <vector>
#include <bitset>
#include <sstream>

namespace TrackerCalib {

  /** 
   * Template to create a string with the bitfield of an integer, 
   * with bits shown in groups. LSB is right-most. 
   *
   * @param[in] bits  std::bitset of the corresponding integer
   * @param[in] step  The step value to group bits.     
   */    
  template<std::size_t N>
    const std::string split(std::bitset<N> bits, int step=4){
    std::stringstream ss;
    ss << bits;
    
    const int n = N / step;
    std::ostringstream ostr;
    std::string::size_type pos(0);
    for(int i=0; i<n; i++){
      ostr << (ss.str()).substr(pos,4);
      if( i<= (n-2) ) ostr << " ";
      pos +=step;
    }
    return ostr.str();
  }

  /**
     \brief enum for data compression mode of ABCD chip.
    
     HIT   : 1XX | X1X | XX1
     LEVEL : X1X
     EDGE  : 01X
     TEST  : XXX
  */
  enum class ABCD_ReadoutMode { HIT=0, LEVEL, EDGE, TEST };
  
  /// forward includes from within namespace TrackerCalib
  class Mask;
  class Trim;
  
  /** *****************************************************************************
   *  Class describing a chip of a SCT module
   ***************************************************************************** */
  class Chip {

  public:
    /** Constructor */
    Chip();

    /** Copy constructor */
    Chip(const Chip&);

    /** operator = (typically used in copy constructor) */
    Chip &operator=(const Chip&);

    /** Destructor, currently empty */
    ~Chip();

    /*---------------------------
     * Set functions
     *-------------------------*/
    
    /** Set address function */
    void setAddress(unsigned int address) { m_address = address; }

    /** Set configuration register function */
    void setCfgReg(unsigned int cfgReg);

    /** Set bias register function */
    void setBiasReg(unsigned int biasReg) { m_biasReg = biasReg; }

    /** Set readout mode (compression criteria).
     * 
     * The four different compression criteria available are defined 
     * with the enum ABCD_ReadoutMode:
     *    0: HIT (1XX | X1X | XX1)
     *    1: LEVEL (X1X)
     *    2: EDGE (01X)
     *    3: TEST (XXX)
     */
    void setReadoutMode(ABCD_ReadoutMode mode);

    /** set calMode <2-3> in configuration register */
    void setCalMode(unsigned int calmode);
    
    /** set trimRange <4-5> in configuration register 
	and update private data member m_trimRange.*/
    void setTrimRange(unsigned int trimRange);

    /** set edge_detect bit (6) in configuration register. */
    void setEdge(bool edge);
    
    /** set Mask bit (7) in configuration register. */
    void setMask(bool mask);

    /** update strobe delay register with SD value */
    void setStrobeDelay(unsigned int strobeDelay);

    /** update threshold/calibration register with calibration amplitude */
    void setCalAmp(float fC);

    /** update threshold/calibration register with threshold value */
    void setThreshold(float mV);

    /** set trim DAC value for channel */
    void setTrimDac(unsigned int channel, unsigned int trimDac);

    /** Set mask flag for specified channel. */
    void setChannelMask(unsigned int channel, int mask);

    /** Set verbosity level */
    void setPrintLevel(int printLevel) { m_printLevel = printLevel; }

     /** Set trim target */
    void setTrimTarget(double val) { m_trimTarget = val; }

    /** Set p0 parameter */
    void setP0(double val) { m_p0 = val; }

    /** Set p1 parameter */
    void setP1(double val) { m_p1 = val; }

    /** Set p2 parameter */
    void setP2(double val) { m_p2 = val; }

    /*---------------------------
     * Get functions
     *-------------------------*/
    
    /** \brief Get address */
    unsigned int address() const { return m_address; }

    /** \brief Get configuration register 
     * 
     * Returns the configuration register for this chip. 
     * This is typically needed to create a proper ConfigHandlerSCT
     * object before configuring the SCT modules via the TRBAccess class.
     */
    unsigned int cfgReg() const { return m_cfgReg; }

    /** Get value of bias register. */
    unsigned int biasReg() const { return m_biasReg; }

    /** Get value of threshold/calibration register  */
    unsigned int threshcalReg() const { return m_threshcalReg; }

    /** Get value of strobe delay register  */
    unsigned int strobeDelayReg() const { return m_strobeDelayReg; }

    /** Get readoutMode  */
    ABCD_ReadoutMode readoutMode() const;

    /** Get trimRange from configuration register */
    unsigned int trimRange() const { return m_trimRange; }

    /** Returns true if edge_detect bit is set in configuration register */
    bool edge() const;

    /** Returns true if mask bit is set in configuration register */
    bool mask() const;
    
    /** Returns the strobe delay value stored in the Strobe Delay register*/
    unsigned int strobeDelay() const;

    /** Returns the calibration amplitude [fC] stored in the threshold/calibration register*/
    float calAmp() const;

    /** Returns the current threshold [mV] stored in the threshold/calibration register*/
    float threshold() const;

    /** Returns the total number of masked channels in chip. */
    int nMasked() const;

     /** Get trim target */
    double trimTarget() const { return m_trimTarget; }
    
    /** Get parameter p0 from RC fit */
    double p0() const { return m_p0; }

    /** Get parameter p1 from RC fit */
    double p1() const { return m_p1; }
    
    /** Get parameter p2 from RC fit */
    double p2() const { return m_p2; }

       /** 
     * Returns if a given channel is flagged as masked.
     * @param[in] channel Channel to be checked.
     */
    bool isChannelMasked(unsigned int channel) const;

    /*---------------------------
     * Misc functions
     *-------------------------*/

     /** Compute the equivalent threshold in mV corresponding to 
	the input charge in fC passed as function argument. */
    double fC2mV(double fC); 
    
    /** Compute the equivalent threhold in fC corresponding to the 
	input threshold in fC passed as function argument. */
    double mV2fC(double mV); 

    /** Get link number associatd with this chip:
	- Link 0: chip addresses [32 ; 37]
	- Link 1: chip addresses [40 ; 45]
    */
    int link() const { return m_address >= 40; }
    
    /** Get chip index within link. For example:
	- address 32 corresponds to [Link 0, ChipIdx 0]
	- address 37 corresponds to [Link 0, ChipIdx 5]
	- address 42 corresponds to [Link 1, ChipIdx 2]
    */
    int chipIdx() const { 
      return m_address < 40 ? m_address - 32 : m_address - 40;
    }

    /** Set required values in vector m_vmask */
    void prepareMaskWords();

    // encode only in m_vmask[0] !!!
    void enableAllChannelsMaskWords();

    /* \brief Returns 16bit words required to apply channel mask
     * together with 1 out of 4 mask for a given calmode.
     * 
     * @param[in] ical  Calibration line in which charge will be injected
     */
    std::vector<uint16_t> getMaskWords(unsigned int ical) const {
      return m_vmask[ical];
    }

    /** get TrimDac value for specified channel */
    unsigned int trimDac(unsigned int channel) const;

    /** get trimWord for specified channel */
    unsigned int trimWord(unsigned int channel) const;

    /**
     * Show attributes of chip
     * @param[in] indent  Indent level of printout
     */
    const std::string print(int indent=0);
    
    /**
     * Show single-channel TrimDac values
     * @param[in] indent  Indent level of printout
     */
    const std::string printTrim(int indent=0);

  private:    
    unsigned int m_address; /// chip address
    unsigned int m_cfgReg; /// contents of configuration register
    unsigned int m_biasReg; /// contents of bias register
    unsigned int m_threshcalReg; /// contents of the threshold/calibration register
    unsigned int m_strobeDelayReg; /// contents of strobe-delay register
    unsigned int m_trimRange; /// trim range

    double m_trimTarget; // trimming target [mV]
    double m_p0; /// RC fit parameter p0
    double m_p1; /// RC fit parameter p1
    double m_p2; /// RC fit parameter p2

    /** mask array for all channels in chip. 
     *   0: channel not masked.
     *   1: channel     masked.
     */
    int m_mask[128];

    /* Array of size 4 corresponding to the mask words combining the mask needed 
       for each calibration line with the channel mask for this chip. */
    std::vector<uint16_t> m_vmask[4];

    std::vector<Trim*> m_trim; /// vector of Trim data

    int m_printLevel; /// Verbosity level

  };  // class Chip


  //  //  bool func(Trim trim, unsigned int channel){
  ///  return trim.channel() == channel;

} // namespace TrackerCalib

#endif 
