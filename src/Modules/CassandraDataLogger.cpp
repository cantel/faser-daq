#include "modules/CassandraDataLogger.hpp"
#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "utilities/ChunkedStorage.hpp"

#include <cassandra.h>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

extern "C" CassandraDataLogger *create_object()
{
    return new CassandraDataLogger;
}

extern "C" void destroy_object(CassandraDataLogger *object)
{
    delete object;
}

CassandraDataLogger::CassandraDataLogger()
{
  INFO(__METHOD_NAME__ << " setting up session.");
  m_cluster = cass_cluster_new();
  m_session = cass_session_new();
  setup();
}

CassandraDataLogger::~CassandraDataLogger()
{
  INFO(__METHOD_NAME__ << " Closing session and connection to cluster...");
  CassFuture* future = cass_session_close( m_session );
  cass_future_wait( future );
  cass_future_free( future );
  cass_cluster_free( m_cluster );
  INFO(__METHOD_NAME__ << " Cassandra connection closed.");
}

const std::string CassandraDataLogger::getErrorStr( CassFuture*& future ) 
{
  CassError rc = cass_future_error_code( future );
  std::string errStr( cass_error_desc(rc) );
  const char* message;
  size_t message_length;
  cass_future_error_message(future, &message, &message_length);
  std::string mssgStr(message, message_length);
  std::string finalStr = errStr + "  Cassandra Message: " + mssgStr;
  return finalStr;
} 

#warning RS -> YOU NEED TO INTRODUCE A PROPER SESSION LAYER BETWEEN STORAGE AND DAQ!

void CassandraDataLogger::start()
{
    INFO("CassandraDataLogger::start");
}

void CassandraDataLogger::stop()
{
    INFO("CassandraDataLogger::stop");
}

void CassandraDataLogger::runner()
{
    
}

void CassandraDataLogger::setup() 
{
  INFO(__METHOD_NAME__ << " Connecting to storage cluster based on configuration.");
  std::string clusterStr = m_config.get<std::string>("ring");

  INFO(__METHOD_NAME__ << " -> connecting to ring: " << clusterStr);
  cass_cluster_set_write_bytes_high_water_mark(m_cluster, 10485760); // Write bytes water mark set to 10MByte

  cass_cluster_set_contact_points( m_cluster, clusterStr.c_str() );
  cass_cluster_set_token_aware_routing(m_cluster, cass_true);

  CassFuture* future = cass_session_connect( m_session, m_cluster );
  cass_future_wait( future );
  if (cass_future_error_code(future) != CASS_OK) {
    ERROR(__METHOD_NAME__ << " Unable to connect to Cassandra cluster for: " << clusterStr
      << " CassError:" << getErrorStr(future));
  } else {
    INFO(__METHOD_NAME__ << " *wink wink* ");
  } 
  cass_future_free( future );
  
}

void CassandraDataLogger::write()
{

}

void CassandraDataLogger::read()
{

}

void CassandraDataLogger::shutdown()
{

}


