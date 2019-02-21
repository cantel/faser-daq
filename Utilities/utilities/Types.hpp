#ifndef DAQ_UTILITIES_TYPES_HPP_
#define DAQ_UTILITIES_TYPES_HPP_
/*
 * Types
 * Author: Roland.Sipos@cern.ch
 * Description: 
 *   Collection of custom types used in the DAQ.
 * Date: May 2018
*/

namespace daq{
namespace utilities{

typedef std::uint64_t timestamp_t;

static const timestamp_t ns = 1;
static const timestamp_t us = 1000 * ns;
static const timestamp_t ms = 1000 * us;
static const timestamp_t s  = 1000 * ms;

}
}


#endif

