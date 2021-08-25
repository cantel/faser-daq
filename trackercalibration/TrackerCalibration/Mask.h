#ifndef __Mask_h__
#define __Mask_h__

#include <string>
#include <vector>
#include <bitset>

namespace TrackerCalib {

  /**
   * Class used to hold the mask values for a range of strip channels of a SCT module.
   *
   * For a single channel, a value of:
   *    0 means channel is     masked.
   *    1 means channel is NOT masked.
   *
   *  This class is thought to cover 16 channels, so that in total a chip would
   *  have 8 instances of this class to cover the 128 channels / chip. 
   */
  class Mask {

  public:
    /** Constructor */
    Mask();
    
    /** Constructor from parameters */
    Mask(unsigned int first, unsigned int last, unsigned int value);

    /** Copy constructor */
    Mask(const Mask&);

    /** operator = (typically used in copy constructor) */
    Mask &operator=(const Mask&);

    /** Destructor, currently empty */
    ~Mask(){};

    /** \brief print function */
    const std::string print(int indent=0);

    /** \brief Function returning first channel of this mask class. */
    unsigned int first() const { return m_first; }
    
    /** \brief Function returning last channel of this mask class. */
    unsigned int last() const { return m_last; }
    
    /** \brief Function returning the hex value of this mask class.  */
    unsigned int value() const { return m_value; }

  private:    
    unsigned int m_first; /// first channel 
    unsigned int m_last;  /// last channel
    unsigned int m_value; /// mask value (hex)
  };  // class Mask

} // namespace TrackerCalib

#endif
