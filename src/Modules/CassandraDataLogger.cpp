#include "modules/CassandraDataLogger.hpp"
#include "utilities/Logging.hpp"
#include "utilities/Common.hpp"
#include "utilities/ChunkedStorage.hpp"

#include <chrono>
#include <sstream>
#include <cassandra.h>

#define __METHOD_NAME__ daq::utilities::methodName(__PRETTY_FUNCTION__)
#define __CLASS_NAME__ daq::utilities::className(__PRETTY_FUNCTION__)

using namespace std::chrono_literals;
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
  const CassPrepared* prepared = nullptr;
  //std::string qStr = "SELECT columnfamily_name FROM system.SCHEMA_COLUMNFAMILIES WHERE keyspace_name=? AND columnfamily_name=?";
  std::string qStr = Q_CF_EXISTS;
  if (prepareQuery(qStr, &prepared)) {
    CassStatement* statement = cass_prepared_bind( prepared );
    cass_statement_bind_string( statement, 0, M_KEYSPACE_NAME.c_str() );
    cass_statement_bind_string( statement, 1, columnFamilyName.c_str() );

    const CassResult* result = nullptr;
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

bool CassandraDataLogger::exists()
{
  return columnFamilyExists( M_CF_NAME ) && m_chunkProvider.exists();
}

bool CassandraDataLogger::create()
{
/*
  std::stringstream qssChunk;
  qssChunk << "CREATE TABLE " << KEYSPACE_NAME<< "." << M_NAME
           << " (" << M_COLUMN_COMPKEY   << " text, "  
                   << M_COLUMN_DATA      << " blob, "
                   << M_COLUMN_OBJSIZE   << " bigint, "
                   << M_COLUMN_CHSIZE    << " bigint, "
                   << M_COLUMN_CHCOUNT   << " int, "
                   << M_COLUMN_EXP       << " int, "	
                   << M_COLUMN_ATTR      << " text, "
           << " PRIMARY KEY (" << M_COLUMN_COMPKEY << "));";
  m_cs->executeQuery( qssChunk.str() );

  std::stringstream qssChunkMeta;
  qssChunkMeta << "CREATE TABLE conddb." << M_META_NAME
               << " (" << M_META_COLUMN_OBJNAME << " text, "
                       << M_COLUMN_OBJSIZE      << " bigint, "
                       << M_COLUMN_CHSIZE       << " bigint, "
                       << M_COLUMN_CHCOUNT      << " int, "
                       << M_META_COLUMN_TTL     << " bigint, "
                       << M_META_COLUMN_PPATH   << " text, "
                       << M_META_COLUMN_ATTR    << " text, "
               << " PRIMARY KEY (" << M_META_COLUMN_OBJNAME << "));";
  m_cs->executeQuery( qssChunkMeta.str() );
*/

  if( exists() ){
    WARNING(__METHOD_NAME__ << " ColumnFamily exists! Won't recreate payload CF.");
    return false;
  }
  std::stringstream qss;
  qss << "CREATE TABLE " << M_KEYSPACE_NAME << "." << M_CF_NAME
      << " (" << M_COLUMN_KEY     << " bigint, "
              << M_COLUMN_TYPE    << " text, "
              << M_COLUMN_SINFO   << " blob, "
              << M_COLUMN_VERSION << " text, "
              << M_COLUMN_TIME    << " bigint, "
              << M_COLUMN_SIZE    << " bigint, "
              << M_COLUMN_DATA    << " blob, " 
      << " PRIMARY KEY (" << M_COLUMN_KEY << "));";
  executeQuery( qss.str() );
  m_chunkProvider.create();
}

#warning RS -> YOU NEED TO INTRODUCE A PROPER SESSION LAYER BETWEEN STORAGE AND DAQ!

void CassandraDataLogger::start()
{
  INFO(__METHOD_NAME__ << " getState: " << getState() );
  m_run = true;
  m_runner_thread = std::make_unique<std::thread>(&CassandraDataLogger::runner, this);
}

void CassandraDataLogger::stop()
{
  m_run = false;
  INFO(__METHOD_NAME__ << " getState: " << this->getState() );
}

void CassandraDataLogger::runner()
{
  auto &cm = daq::core::ConnectionManager::instance();
  while (m_run)
  {
    INFO(__METHOD_NAME__ << " Running...");
    std::this_thread::sleep_for(500ms);
    INFO("Received on channel 1 " << cm.getStr(1));
  }
  INFO(__METHOD_NAME__ << " Runner stopped");    
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
  INFO(__METHOD_NAME__ << " Checking PAYLOAD ColumnFamily existence...");
  if ( exists() ){
    INFO(__METHOD_NAME__ << "   -> CF is up.");
  } else {
    bool done = create();
    INFO(__METHOD_NAME__ << "   -> CF created -> " << done);
  }
  cass_future_free( future );
  
}

void CassandraDataLogger::write()
{

}


void CassandraDataLogger::read()
{

}

bool CassandraDataLogger::write(uint64_t keyId, daq::utilities::Binary& payload)
{
  daqutils::Binary sinfoData(0);
  bool success = false;
  const CassPrepared* prepared = nullptr; 
  if ( prepareQuery(Q_INSERT, &prepared) ) {
    CassStatement* statement = cass_prepared_bind( prepared );
    cass_statement_bind_int64( statement, 0, keyId );
    cass_statement_bind_string( statement, 1, "event" );
    cass_statement_bind_string( statement, 3, "dummy" );
    cass_statement_bind_int64( statement, 4, 0L );
    cass_statement_bind_bytes( statement, 2, static_cast<const unsigned char*>(sinfoData.data()), sinfoData.size() );
#ifndef USE_CHUNKS
    cass_statement_bind_bytes( statement, 6, static_cast<const unsigned char*>(payload.data()), payload.size() );
#else
    //cond::Binary dummyPayload = cond::Binary(); 
    //cass_statement_bind_bytes( statement, 6, reinterpret_cast<const unsigned char*>(dummyPayload.data()), dummyPayload.size() ); 
    //ObjectMetadata meta = ChunkedStorage::newWriter(m_chunkSP, payloadHash, payloadData).call();
#endif
    cass_statement_bind_int64( statement, 5, payload.size() ); 
    success = executeStatement(statement);
    cass_statement_free( statement );
    success = true;
  }
  cass_prepared_free( prepared );
  return success;
}



void CassandraDataLogger::shutdown()
{

}


