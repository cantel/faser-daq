#ifndef DAQ_MODULES_CASSANDRADATALOGGER_HPP_
#define DAQ_MODULES_CASSANDRADATALOGGER_HPP_

/// \cond
#include <iostream>
#include <cassandra.h>
/// \endcond

#include "Core/DAQProcess.hpp"
#include "Core/DataLogger.hpp"
#include "Utilities/Binary.hpp"
#include "Utilities/ChunkedStorage.hpp"


//#define HASH_MODE
#define EVID_MODE

/*
 * CassandraDataLogger
 * Author: Roland.Sipos@cern.ch
 * Description: Data logger with Cassandra persistency layer
 *   Heavily relies on the CondDB payload chunked storage.
 * Date: November 2017
 */
class CassandraDataLogger : public DAQProcess, public DataLogger {
  class CassandraChunkedStorageProvider : public daq::persistency::ChunkedStorageProvider {
   public:
    explicit CassandraChunkedStorageProvider(...) {
      // Any init?
    }

    virtual ~CassandraChunkedStorageProvider(){};
    bool exists() { return true; }
    void create(){};
    const size_t writeChunk(const std::string& objectName, int chunkId,
                            const std::pair<const void*, size_t>& data, int ttl) const {
      return 0;
    }
    bool readChunk(const std::string& objectName, int chunkId, size_t split, void*& blobPtr) const {
      return false;
    };
    const int getDefaultChunkSize() { return 0; }
    void deleteObject(const std::string& objectName, int chunkCount) const {}
    void writeMetadata(const std::string& objectName,
                       const daq::persistency::ObjectMetadata& attr) const {}
    const daq::persistency::ObjectMetadata readMetadata(const std::string& objectName) const {
      daq::persistency::ObjectMetadata attributes;
      return attributes;
    }

   private:
    const std::string M_NAME = "chunk";
  };


  public:
    CassandraDataLogger();
    ~CassandraDataLogger();

    void start();
    void stop();
    void runner();

    void setup();
    void write();
    void read();
    bool write(uint64_t keyId, daq::utilities::Binary& payload);
    void shutdown();

  private:

// RS -> ALL THIS SHOULD BE NICELY HIDDEN BEHIND A SESSION LAYER.
    const std::string M_KEYSPACE_NAME = "daq";
    const std::string M_CF_NAME = "payload";

    const std::string M_COLUMNFAMILY  = "pkey, type, s_info, version, time, size, data";
    const std::string M_COLUMN_KEY     = "pkey";
    const std::string M_COLUMN_TYPE    = "type";
    const std::string M_COLUMN_SINFO   = "s_info";
    const std::string M_COLUMN_VERSION = "version";
    const std::string M_COLUMN_TIME    = "time";
    const std::string M_COLUMN_SIZE    = "size";
    const std::string M_COLUMN_DATA    = "data";

    const std::string Q_SAY_HI = "SELECT key,bootstrapped,broadcast_address,cluster_name,cql_version,data_center FROM system.local";   
    const std::string Q_CF_EXISTS = "SELECT table_name from system_schema.tables WHERE keyspace_name=? AND table_name=?";
    const std::string Q_INSERT = "INSERT INTO daq.payload (pkey, type, s_info, version, time, size, data) VALUES (?,?,?,?,?,?,?);";

    const std::string getErrorStr( CassFuture*& future );
    void readIntoBinary( daq::utilities::Binary& binary, const CassValue* const & value );
    bool prepareQuery( const std::string& qStr, const CassPrepared** prepared );
    bool executeStatement( CassStatement*& statement );
    bool executeStatement( CassStatement*& statement, const CassResult** result );
    bool executeQuery( const std::string& queryStr );
    bool columnFamilyExists( const std::string& cfName ); 

    bool exists();
    bool create();

    CassandraChunkedStorageProvider m_chunkProvider;

    CassCluster* m_cluster;
    CassSession* m_session;


};

#endif /* DAQ_MODULES_CASSANDRADATALOGGER_HPP_ */
