#ifndef DAQ_UTILITIES_HASH_h
#define DAQ_UTILITIES_HASH_h

#include "utilities/Logging.hpp"
#include "utilities/Binary.hpp"

#include <openssl/sha.h>

/*
 * Hash
 * Author: Roland.Sipos@cern.ch
 * Description: 
 *   Hash utils from CondDB
 * Date: March 2019
*/
namespace daq {

  namespace persistency {

    typedef std::string Hash;
    static constexpr unsigned int HASH_SIZE = 40;

    inline Hash makeHash( const std::string& objectType, const daq::utilities::Binary& data ){
      SHA_CTX ctx;
      if( !SHA1_Init( &ctx ) ){
        ERROR("daq::persistency::makeHash: SHA1 initialization error.");
      }
      if( !SHA1_Update( &ctx, objectType.c_str(), objectType.size() ) ){
        ERROR("daq::persistency::makeHash: SHA1 processing error (1).");
      }
      if( !SHA1_Update( &ctx, data.data(), data.size() ) ){
        ERROR("daq::persistency::makeHash: SHA1 processing error (2).");
      }
      unsigned char hash[SHA_DIGEST_LENGTH];
      if( !SHA1_Final(hash, &ctx) ){
        ERROR("daq::persistency::makeHash: SHA1 finalization error.");
      }
  
      char tmp[SHA_DIGEST_LENGTH*2+1];
      // re-write bytes in hex
      for (unsigned int i = 0; i < 20; i++) {
        ::sprintf(&tmp[i * 2], "%02x", hash[i]);
      }
      tmp[20*2] = 0;
      return tmp;
    }

  }

}

#endif

