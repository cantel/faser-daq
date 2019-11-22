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
  
  void registerHistogram( std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, float delta_t = 60. ) {
    INFO("Registering histogram "<<name);

    auto interval_in_s = m_interval/1000.;

    if ( delta_t < interval_in_s ){
      delta_t = m_interval ;
      INFO("publishing interval cannnot be set below "<<interval_in_s<<" s. Setting publishing interval to "<<interval_in_s<<" s."); 
    }
    HistBase * hist = new Hist<hist_t>(name, xlabel, xmin, xmax, xbins, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));

    return;
  }

  void registerHistogram( std::string name, std::string xlabel, std::vector<std::string> categories, float delta_t ){
    INFO("Registering histogram "<<name);

    HistBase * hist = new Hist<categoryhist_t>(name, xlabel, categories, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));
  
    return;
  }

  void register2DHistogram( std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, std::string ylabel, float ymin, float ymax, unsigned int ybins, float delta_t = 60. ) {
    INFO("Registering histogram "<<name);
    
    auto interval_in_s = m_interval/1000.;

    if ( delta_t < interval_in_s ){
      delta_t = m_interval ;
      INFO("publishing interval cannnot be set below "<<interval_in_s<<" s. Setting publishing interval to "<<interval_in_s<<" s."); 
    }
    HistBase * hist2D = new Hist2D(name, xlabel, xmin, xmax, xbins, ylabel, ymin, ymax, ybins, delta_t);
    m_histogram_map.insert( std::make_pair( hist2D->name, hist2D));

    return;
  }

  void publish( HistBase * h);

  template<typename X>
  void fill( std::string name, X value ){
    
    static_assert(std::is_integral<X>::value || std::is_floating_point<X>::value,
                  "Cannot fill histogram with invalid value type. Value must be numeric or string based (std::string or const char *)");

    if ( m_histogram_map.count(name) ) {
      static_cast<Hist<hist_t>*>(m_histogram_map[name])->fill(value);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  void fill( std::string name, std::string value )  {
    
    if ( m_histogram_map.count(name) ) {
      static_cast<Hist<categoryhist_t>*>(m_histogram_map[name])->fill(value);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  void fill( std::string name, const char * value )  {
    
    if ( m_histogram_map.count(name) ) {
      static_cast<Hist<categoryhist_t>*>(m_histogram_map[name])->fill(value);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  template<typename X, typename Y>
  void fill( std::string name, X xvalue, Y yvalue ){
    
    static_assert(std::is_integral<X>::value || std::is_floating_point<X>::value || std::is_same<X, std::string>::value || std::is_same<X, const char *>::value,
                  "Cannot fill histogram with invalid  x value type. Value must be numeric or string based (std::string or const char *)");
    static_assert(std::is_integral<Y>::value || std::is_floating_point<Y>::value || std::is_same<Y, std::string>::value || std::is_same<Y, const char *>::value,
                  "Cannot fill histogram with invalid  y value type. Value must be numeric or string based (std::string or const char *)");

    if ( m_histogram_map.count(name) ) {
     static_cast<Hist2D*>(m_histogram_map[name])->fill(xvalue, yvalue);
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
