#pragma once

#include "zmq.hpp"
#include <thread>
#include <atomic>
#include <ctime>
#include <iostream>
#include <map>
#include <type_traits> // is_integral, is_floating_point, ...

#include "Utils/Histogram.hpp"
#include "Utils/Logging.hpp"
using namespace boost::histogram;

class HistogramManager{
public:

 
  HistogramManager(std::unique_ptr<zmq::socket_t>& statSock, unsigned interval=500);
  
  ~HistogramManager();

  bool configure(unsigned interval);

  void setZMQpublishing(bool zmq_publisher){m_zmq_publisher = zmq_publisher;}

  void start();
  
  void registerHistogram( std::string name, std::string xlabel, float start_range, float end_range, unsigned int number_bins, float delta_t = 60. ) {
    INFO("Registering histogram "<<name);

    if ( delta_t < m_interval/1000. ){
      delta_t = m_interval ;
      INFO("publishing interval cannnot be set below "<<m_interval/1000.<<" s. Setting publishing interval to "<<m_interval/1000.<<" s."); 
    }
    HistBase * hist = new Hist<hist_t>(name, xlabel, start_range, end_range, number_bins, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));

    return;
  }

  void registerHistogram( std::string name, std::string xlabel, std::vector<std::string> categories, float delta_t ){
    INFO("Registering histogram "<<name);

    HistBase * hist = new Hist<categoryhist_t>(name, xlabel, categories, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));
  
    return;

  }

  void publish( HistBase * h);

  template<typename X>
  void fill( std::string name, X value ){
    
    static_assert(std::is_integral<X>::value || std::is_floating_point<X>::value || std::is_same<X, std::string>::value || std::is_same<X, const char *>::value,
                  "Cannot fill histogram with invalid value type. Value must be numeric or string based (std::string or const char *)");

    if ( m_histogram_map.count(name) ) {
      m_histogram_map[name]->fill(value);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  private:

  // Thread control
  std::thread m_histogram_thread;
  std::atomic<bool> m_stop_thread;

  // Config for data publishing
  std::atomic<bool> m_zmq_publisher;

  // Publish socket ref for hists
  std::unique_ptr<zmq::socket_t>& m_stat_socket;

  // Config
  unsigned m_interval;
  std::map< std::string, HistBase * > m_histogram_map;

  void CheckHistograms();


};
