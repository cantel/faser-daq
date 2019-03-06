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

#endif

