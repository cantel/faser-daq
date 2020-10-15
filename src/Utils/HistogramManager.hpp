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
#include "Core/Configuration.hpp"

using namespace boost::histogram;

namespace Axis {
  enum Range{
   NONEXTENDABLE=0,
   EXTENDABLE=1
  };
}

class HistogramManager{
public:

 
  HistogramManager(std::unique_ptr<zmq::socket_t>& statSock, unsigned interval=500);
  
  ~HistogramManager();

  bool configure(unsigned interval);

  void setZMQpublishing(bool zmq_publisher){m_zmq_publisher = zmq_publisher;}

  void start();

  void stop();
  
  void registerHistogram( std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, Axis::Range extendable, unsigned int delta_t = 60) {
    INFO("Registering histogram "<<name);

    auto interval_in_s = m_interval/1000.;

    if ( delta_t < interval_in_s ){
      delta_t = m_interval ;
      INFO("publishing interval cannnot be set below "<<interval_in_s<<" s. Setting publishing interval to "<<interval_in_s<<" s."); 
    }
  
    HistBase * hist;
    if (extendable)
      hist = new Hist<stretchy_hist_t>(name, xlabel, ylabel, xmin, xmax, xbins, Axis::Range::EXTENDABLE, delta_t);
    else 
      hist = new Hist<hist_t>(name, xlabel, ylabel, xmin, xmax, xbins, Axis::Range::NONEXTENDABLE, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));

    return;
  }

  void registerHistogram( std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, unsigned int delta_t = 60 ) {
    registerHistogram( name, xlabel, "counts", xmin, xmax, xbins, Axis::Range::NONEXTENDABLE, delta_t); 
    return;
  }

  void registerHistogram( std::string name, std::string xlabel, std::string ylabel, float xmin, float xmax, unsigned int xbins, unsigned int delta_t = 60 ) {
    registerHistogram( name, xlabel, ylabel, xmin, xmax, xbins, Axis::Range::NONEXTENDABLE, delta_t); 
    return;
  }

  void registerHistogram( std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, Axis::Range extendable, unsigned int delta_t = 60 ) {
    registerHistogram( name, xlabel, "counts", xmin, xmax, xbins, extendable, delta_t); 
    return;
  }

  void registerHistogram( std::string name, std::string xlabel, std::string ylabel, std::vector<std::string> categories, unsigned int delta_t = 60 ){
    INFO("Registering histogram "<<name);

    HistBase * hist = new CategoryHist(name, xlabel, ylabel, categories, delta_t);
    m_histogram_map.insert( std::make_pair( hist->name, hist));
  
    return;
  }

  void registerHistogram( std::string name, std::string xlabel, std::vector<std::string> categories, unsigned int delta_t = 60 ){
    registerHistogram( name, xlabel, "counts", categories, delta_t);
    return;
  }

  void register2DHistogram( std::string name, std::string xlabel, float xmin, float xmax, unsigned int xbins, std::string ylabel, float ymin, float ymax, unsigned int ybins, unsigned int delta_t = 60. ) {
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

  template<typename X, typename W = int>
  void fill( std::string name, X value, W weight=1 ){
    
    static_assert(std::is_integral<X>::value || std::is_floating_point<X>::value,
                  "Cannot fill histogram with invalid value type. Value must be numeric or string based (std::string or const char *)");
    static_assert(std::is_integral<W>::value || std::is_floating_point<W>::value,
                  "Cannot fill histogram with invalid weight value type. Value must be numeric.");

    if ( m_histogram_map.count(name) ) {
      HistBase * hist = m_histogram_map[name];
      if (hist->extendable)
        static_cast<Hist<stretchy_hist_t>*>(hist)->fill(value, weight);
      else 
        static_cast<Hist<hist_t>*>(hist)->fill(value, weight);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  template<typename X, typename W = int>
  void fill( std::string name, std::string value, W weight=1 )  {

    static_assert(std::is_integral<W>::value || std::is_floating_point<W>::value,
                  "Cannot fill histogram with invalid weight value type. Value must be numeric.");
    
    if ( m_histogram_map.count(name) ) {
      static_cast<CategoryHist*>(m_histogram_map[name])->fill(value, weight);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  template<typename W = int>
  void fill( std::string name, const char * value, W weight=1 )  {

    static_assert(std::is_integral<W>::value || std::is_floating_point<W>::value,
                  "Cannot fill histogram with invalid weight value type. Value must be numeric.");
    
    if ( m_histogram_map.count(name) ) {
      static_cast<CategoryHist*>(m_histogram_map[name])->fill(value, weight);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  template<typename X, typename Y, typename W = int>
  void fill2D( std::string name, X xvalue, Y yvalue, W weight=1 ){
    
    static_assert(std::is_integral<X>::value || std::is_floating_point<X>::value,
                  "Cannot fill histogram with invalid x value type. Value must be numeric.");
    static_assert(std::is_integral<Y>::value || std::is_floating_point<Y>::value,
                  "Cannot fill histogram with invalid y value type. Value must be numeric.");
    static_assert(std::is_integral<W>::value || std::is_floating_point<W>::value,
                  "Cannot fill histogram with invalid weight value type. Value must be numeric.");

    if ( m_histogram_map.count(name) ) {
     static_cast<Hist2D*>(m_histogram_map[name])->fill(xvalue, yvalue, weight);
    }
    else 
      WARNING("Histogram with name "<<name<<" does not exist.");
  }

  void resetOnPublish(std::string, bool);

  void reset(std::string);

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
  daqling::core::Configuration &m_config = daqling::core::Configuration::instance();
  std::string m_name; // module name

  std::map< std::string, HistBase * > m_histogram_map;

  void CheckHistograms();
  void flushHistograms();

};
