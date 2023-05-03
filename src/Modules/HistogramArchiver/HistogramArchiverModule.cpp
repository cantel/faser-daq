/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#include "HistogramArchiverModule.hpp"
#include "Utils/Ers.hpp"

#include <cstdlib>
#include <memory>
#include <string>
#include <stdexcept>


HistogramArchiverModule::HistogramArchiverModule(const std::string& n):FaserProcess(n) { 
  ERS_INFO(""); 
  auto cfg=getModuleSettings();
  m_filename_pattern=cfg["filename_pattern"];
  m_max_age=cfg["max_age"];
  m_dump_interval=cfg["dump_interval"];
}

HistogramArchiverModule::~HistogramArchiverModule() { 
}

// optional (configuration can be handled in the constructor)
void HistogramArchiverModule::configure() {
  FaserProcess::configure();
  ERS_INFO("");
  char temp [ 1000 ];
  std::string path=getcwd(temp,1000);
  size_t bidx=path.rfind("/");
  m_archive_command=path.substr(0,bidx)+"/scripts/archiveHists.py";
  if (access( m_archive_command.c_str(), X_OK ) != 0 ) {
    m_status=STATUS_ERROR;
    ERS_ERROR("Failed to find archive command or not executable: "+m_archive_command);
  }
}

void HistogramArchiverModule::start(unsigned run_num) {
  FaserProcess::start(run_num);
  ERS_INFO("");
  m_runnumber=run_num;
  m_file_index=0;
  m_last_archive=time(0);
}

void HistogramArchiverModule::stop() {
  archive();
  FaserProcess::stop();
  ERS_INFO("");
}

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}


void HistogramArchiverModule::archive() {
  DEBUG("pattern: "+m_filename_pattern);
  try {
    std::string hist_name=string_format(m_filename_pattern,m_runnumber,m_file_index);
    ERS_INFO("Archiving histograms to "+hist_name);
    std::string full_command=m_archive_command+" "+std::to_string(m_max_age)+" "+hist_name;
    DEBUG("Full command: "+full_command);
    int rc=system(full_command.c_str());
    if (rc) {
      m_status=STATUS_ERROR;
      ERS_ERROR("Got errors during archiving");
    }
    m_last_archive=time(0);
    m_file_index++;
  } catch (std::runtime_error &e) {
    m_status=STATUS_ERROR;
    m_last_archive=time(0);  //to avoid repeated failures every second
    ERS_ERROR("Failed to generate outputfile name");
    return;
  }
}

void HistogramArchiverModule::runner() noexcept {
  ERS_INFO("Running...");
  while (m_run) {
    if (time(0)-m_last_archive>=m_dump_interval) {
      archive();
    }
    sleep(1);
  }
  ERS_INFO("Runner stopped");
}
