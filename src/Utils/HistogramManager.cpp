/*
  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
*/
#include "HistogramManager.hpp"
#include <type_traits>
#include <typeinfo>

HistogramManager::HistogramManager() {
  m_zmq_publisher = false;  
  m_name = m_config.getName();
}


HistogramManager::~HistogramManager(){
  m_histo_socket->setsockopt(ZMQ_LINGER, 1);
  for (std::map<std::string, HistBase *>::iterator hist_itr = m_histogram_map.begin(); hist_itr!=m_histogram_map.end(); hist_itr++){
    delete (hist_itr->second);
  }
  m_histogram_map.clear();
  m_histo_socket.reset();
  m_histo_context.reset();
}

void HistogramManager::configure(uint8_t ioT, std::string connStr, unsigned interval) {
  m_interval = interval;
  try {
    m_histo_context = std::make_unique<zmq::context_t>(ioT); // NOTE using new context, any reason to use daqling stats context?
    m_histo_socket = std::make_unique<zmq::socket_t>(*(m_histo_context.get()), ZMQ_PUB);
    m_histo_socket->connect(connStr);
    INFO(" Histograms are published on: " << connStr);
  } catch (std::exception &e) {
    throw FailedToAddZMQChannel(ERS_HERE,e.what());
  }
  m_zmq_publisher = true;
}

void HistogramManager::start(){

  INFO("HistogramManager thread about to spawn...");
  
  m_stop_thread = false;
  m_histogram_thread = std::thread(&HistogramManager::CheckHistograms,this);
}

void HistogramManager::stop(){

  INFO("HistogramManager shutting down ...");
  m_stop_thread = true;
  if (m_histogram_thread.joinable())
    m_histogram_thread.join();

  INFO("Flushing all histograms ...");
  flushHistograms();

  // m_histo_socket.reset();
  // m_histo_context.reset();
  m_zmq_publisher = false;

  return;
}

void HistogramManager::CheckHistograms(){

  while(!m_stop_thread){
    for (auto &pair : m_histogram_map ) { 
      HistBase * x = pair.second;
      if(std::difftime(std::time(nullptr), x->timestamp) >= x->delta_t){
            publish(x);
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(m_interval));
  }
}

void HistogramManager::flushHistograms(){

  for (auto &pair : m_histogram_map ) { 
    HistBase * x = pair.second;
    publish(x);
  }

}

void HistogramManager::publish( HistBase * h){

  std::ostringstream msg;
  msg<<m_name+"-h_"+h->name<<": "<<h->publish();
  DEBUG("START_OF_MSG:" <<std::endl << msg.str());
  if(m_zmq_publisher){
     zmq::message_t message(msg.str().size());
     memcpy (message.data(), msg.str().data(), msg.str().size());
     bool rc = m_histo_socket->send(message);
     if(!rc)
        WARNING("Failed to publish histogram with name "<<h->name);
   }
  h->timestamp = std::time(nullptr);
 
  return;
}

void HistogramManager::resetOnPublish(std::string name, bool reset = true){
  if ( m_histogram_map.count(name) ) {
    m_histogram_map[name]->reset_on_publish(reset);
  }
  else 
      WARNING("Histogram with name "<<name<<" does not exist.");
 
  return;
}

void HistogramManager::reset(std::string name){
  if ( m_histogram_map.count(name) ) {
    m_histogram_map[name]->reset();
  }
  else 
      WARNING("Histogram with name "<<name<<" does not exist.");
 
  return;
}
