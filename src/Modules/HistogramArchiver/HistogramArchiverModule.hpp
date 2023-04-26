/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/

#pragma once
#include <cstdlib>
#include <ctime>

#include "Commons/FaserProcess.hpp"

class HistogramArchiverModule : public FaserProcess {
 public:
  HistogramArchiverModule(const std::string& n);
  ~HistogramArchiverModule();

  void configure(); // optional (configuration can be handled in the constructor)
  void archive();
  void start(unsigned);
  void stop();

  void runner() noexcept;
private:
  unsigned m_runnumber;
  std::string m_archive_command;
  std::string m_filename_pattern;
  int m_max_age;       // in seconds
  int m_dump_interval; // in seconds
  
  int m_file_index;
  time_t m_last_archive;
};
