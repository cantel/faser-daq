#include "modules/CassandraDataLogger.hpp"
#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "utilities/ChunkedStorage.hpp"

#include <cassandra.h>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

namespace daqutils = daq::utilities;

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


void CassandraDataLogger::readIntoBinary( daqutils::Binary& binary, const CassValue* const & value ) {
  const cass_byte_t* value_bytes;
  size_t value_length;
  cass_value_get_bytes( value, &value_bytes, &value_length );
  binary = daqutils::Binary( static_cast<const void*>(value_bytes), value_length ); 
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

bool CassandraDataLogger::prepareQuery( const std::string& qStr, const CassPrepared** prepared ) {
  bool success = false;
  CassFuture* future = cass_session_prepare( m_session, qStr.c_str() );
  cass_future_wait( future );
  if(cass_future_error_code(future) != CASS_OK) {
    ERROR(__METHOD_NAME__ << " Query preparation failed for: " << qStr << " CassError: " << getErrorStr(future)); 
    return success;
  } else {
    *prepared = cass_future_get_prepared( future );
    success = true;
  }
  cass_future_free( future );
  return success;
}

bool CassandraDataLogger::executeStatement( CassStatement*& statement ) {
  bool success = false;
  CassFuture* future = cass_session_execute( m_session, statement );
  cass_future_wait( future );
  if (cass_future_error_code(future) != CASS_OK)
    ERROR(__METHOD_NAME__ << " Statement execution failed. CassError: " << getErrorStr(future));
  else
    success = true;
  cass_future_free( future );
  return success;
}

bool CassandraDataLogger::executeStatement( CassStatement*& statement, const CassResult** result ) {
  bool success = false;
  CassFuture* future = cass_session_execute( m_session, statement );
  cass_future_wait( future );
  if (cass_future_error_code(future) != CASS_OK) {
    ERROR(__METHOD_NAME__ << " Statement execution failed. CassError: " << getErrorStr(future));
  } else {
    *result = cass_future_get_result(future);
    success = true;
  }
  cass_future_free( future );
  return success;
}

bool CassandraDataLogger::executeQuery( const std::string& queryStr ) {
  bool success = false;
  CassStatement* statement = cass_statement_new( queryStr.c_str(), 0 );      
  CassFuture* future = cass_session_execute( m_session, statement );
  cass_future_wait( future );
  if (cass_future_error_code(future) != CASS_OK)
    ERROR(__METHOD_NAME__ << " Query execution failed for: " << queryStr << " CassError: " << getErrorStr(future));
  else
    success = true;
  cass_future_free( future );
  cass_statement_free( statement );
  return success;
}

bool CassandraDataLogger::columnFamilyExists( const std::string& columnFamilyName  ) {
  bool found = false;  
  const CassPrepared* prepared = NULL;
  std::string qStr = "SELECT columnfamily_name FROM system.SCHEMA_COLUMNFAMILIES WHERE keyspace_name=? AND columnfamily_name=?";
  if (prepareQuery(qStr, &prepared)) {
    CassStatement* statement = cass_prepared_bind( prepared );
    cass_statement_bind_string( statement, 0, KEYSPACE_NAME.c_str() );
    cass_statement_bind_string( statement, 1, columnFamilyName.c_str() );

    const CassResult* result = NULL;
    if (executeStatement(statement, &result)){
      CassIterator* iterator = cass_iterator_from_result( result ); 
      found = ( cass_iterator_next(iterator) ) ? true : false;
      cass_iterator_free( iterator );
    }
    cass_result_free( result );
    cass_statement_free( statement );
  }
  cass_prepared_free( prepared ); 
  return found;
} 

bool CassandraDataLogger::CassandraChunkedStorageProvider::exists()
{
 return false;  
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


