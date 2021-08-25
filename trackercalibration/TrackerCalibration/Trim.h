#ifndef __Trim_h__
#define __Trim_h__

#include <string>
#include <vector>
#include <bitset>

namespace TrackerCalib {

  /**
   *  Class describing Trim data (channel + TrimDac value)
   */
  class Trim {

  public:
    /** Constructor */
    Trim();

    /* \brief Constructor with parameter
     * 
     * @param[in] channel: channel number
     * @param[in] trimDac: value of the TrimDac register (4 bits)
     */
    Trim(unsigned int channel, unsigned int trimDac);
    
    /** Copy constructor */
    Trim(const Trim&);

    /** operator = (typically used in copy constructor) */
    Trim &operator=(const Trim&);

    /** Destructor, currently empty */
    ~Trim();

    /** Set address function */
    void setChannel(unsigned int channel);

    /** Set trimDac */
    void setTrimDac(unsigned int trimDac);

    /** Update word using function arguments. Internal private members
	will of course be also updated. */
    void setTrimData(unsigned int channel, unsigned int trimDac);

    /** \brief Get channel */
    unsigned int channel() const { return m_channel; }

    /** \brief Get trimDac */
    unsigned int trimDac() const { return m_trimDac; }

    /** \brief Get trimWord */
    unsigned int trimWord() const { return m_trimWord; }
    
    /**
     * Show TrimDac information.
     * @param[in] indent  Indent level of printout
     */
    const std::string print(int indent=0);

  private:
    /** Update word using values of private members */
    void setTrimWord();

  private:    
    unsigned int m_channel; /// channel number
    unsigned int m_trimDac; /// trimDac value    
    unsigned int m_trimWord; /// 16-bit word encoding channel and trimData to be loaded into trim register
  };  // class Trim

} // namespace TrackerCalib

#endif 
