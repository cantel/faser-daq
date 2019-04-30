#ifndef DAQ_UTILITIES_BINARY_HPP
#define DAQ_UTILITIES_BINARY_HPP

/*
 * Binary
 * Added by: Roland.Sipos@cern.ch
 * Description:
 *   A really nice void* wrapper from CORAL
 *   https://twiki.cern.ch/twiki/bin/view/Persistency/Coral 
 * Date: May 2018
*/

#include <iostream>
#include <iomanip>

namespace daq
{
namespace utilities
{

  class Binary
  {

  public:

    /// Default Constructor. Creates an empty BLOB
    Binary();

    /// Constructor initializing a BLOB with initialSize bytes
    explicit Binary( long initialSizeInBytes );

    explicit Binary( const void* data, long size );

    /// Destructor. Frees internally allocated memory
    ~Binary();

    /// Copy constructor
    Binary( const Binary& rhs );

    /// Assignment operator
    Binary& operator=( const Binary& rhs );

    /// Appends the data of another blob
    Binary& operator+=( const Binary& rhs );

    /// Equal operator. Compares the contents of the binary blocks
    bool operator==( const Binary& rhs ) const;

    /// Comparison operator
    bool operator!=( const Binary& rhs ) const;

    /// Returns the starting address of the BLOB
    const void* startingAddress() const;

    /// Returns the starting address of the BLOB
    void* startingAddress();

    const void* data() const;

    /// Current size of the blob
    long size() const;

    /// Extends the BLOB by additionalSizeInBytes
    void extend( long additionalSizeInBytes );

    /// Resizes a BLOB to sizeInBytes
    void resize( long sizeInBytes );

  private:

    /// The current size of the BLOB
    long m_size;

    /// The BLOB data buffer
    void* m_data;

  };

}
}

// Inline methods
inline bool
daq::utilities::Binary::operator!=( const Binary& rhs ) const
{
  return ( ! ( this->operator==( rhs ) ) );
}

inline std::ostream& operator<<(std::ostream& out, const daq::utilities::Binary& rhs)
{
  for (int i = 0; i < rhs.size(); i++) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << (int)*((unsigned char*)rhs.data() + i) << std::dec;
    if (i % 4 == 3)
    {
      if (i % 16 == 15)
      {
        std::cout << "\n";
      }
      else
      {
        std::cout << " ";
      }
    }
  }
  std::cout << "\n";
  return out;
}

#endif

