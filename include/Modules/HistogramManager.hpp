#ifndef HISTOGRAMMANAGER_HPP
#define HISTOGRAMMANAGER_HPP

#include "Modules/Histogram.hpp"
#include "zmq.hpp"
#include <thread>
#include <atomic>
#include <ctime>
#include <iostream>
#include <map>

using namespace boost::histogram;

class HistogramManager{
public:

 
  //HistogramManager(std::unique_ptr<zmq::socket_t>& histSock, unsigned interval=500);
  HistogramManager( unsigned interval=500);
  
  ~HistogramManager();

  bool configure(unsigned interval);

  void setZQMpublishing(bool zmq_publisher){m_zmq_publisher = zmq_publisher;}

  void start();
  
  template <typename U>
  void registerHistogram( std::string name, std::string xlabel, float start_range, float end_range, unsigned int number_bins, float delta_t ) {

    std::cout<<"creating hist with name "<< name<<std::endl;
    HistBase * hist = new Hist<hist_t, U>(name, xlabel, start_range, end_range, number_bins, delta_t);
    //HistBase * hist_base = dynamic_cast<Hist<hist_t>*>(hist);
    std::cout<<"adding to map "<<std::endl;
    m_histogram_map.insert( std::make_pair( hist->name, hist));
    std::cout<<"done!"<<std::endl;

    return;
  }

  template <typename S>
  void registerHistogram( std::string name, std::string xlabel, std::vector<std::string> categories, float delta_t ){

    std::cout<<"creating hist with name "<< name<<std::endl;
    HistBase * hist = new Hist<categoryhist_t, S>(name, xlabel, categories, delta_t);
    //HistBase * hist_base = dynamic_cast<Hist<categoryhist_t>*>(hist);
    std::cout<<"adding to map "<<std::endl;
    m_histogram_map.insert( std::make_pair( hist->name, hist));
    std::cout<<"done!"<<std::endl;
  
    return;

  }

  void publish( HistBase * h);

  template<typename X>
  void fill( std::string name, X value ){
    if ( m_histogram_map.count(name) ) {
      std::cout<<"filling histogram with name "<<name<<std::endl;
      m_histogram_map[name]->fill((void*)&value);
    }
    else 
      std::cout<<"WARNING : histogram with name "<<name<<" does not exist."<<std::endl;	
  }

  private:

  // Thread control
  std::thread m_histogram_thread;
  std::atomic<bool> m_stop_thread;

  // Config for data publishing
  std::atomic<bool> m_zmq_publisher;

  // Publish socket ref for hists
  //std::unique_ptr<zmq::socket_t>& m_hist_socket;

  // Config
  unsigned m_interval;
  std::map< std::string, HistBase * > m_histogram_map;

  void CheckHistograms();


};

#endif // HISTOGRAMMANAGER_HPP
