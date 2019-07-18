/// \cond
#include <chrono>
/// \endcond
using namespace std::chrono_literals;
using namespace std::chrono;

#include <map>
#include <boost/format.hpp>
#include <boost/histogram/ostream.hpp> // write histogram straight to ostream
#include <iostream> // std::flush
#include <sstream> // std::ostringstream
#include <fstream>      // std::ofstream

using namespace boost::histogram;

#include "Modules/TLBMonitor.hpp"

#define __MODULEMETHOD_NAME__ daqling::utilities::methodName(__PRETTY_FUNCTION__)
#define __MODULECLASS_NAME__ daqling::utilities::className(__PRETTY_FUNCTION__)

extern "C" TLBMonitor *create_object() { return new TLBMonitor; }

extern "C" void destroy_object(TLBMonitor *object) { delete object; }

TLBMonitor::TLBMonitor() { 

   INFO("TLBMonitor::TLBMonitor");

   initialize_hists();

   // make this configurable ...?
   m_json_file_name = "tlb_histogram_output.json";

   auto cfg = m_config.getConfig()["settings"];
   m_sourceID = cfg["fragmentID"];
   m_timeblock = cfg["lengthTimeBlock"]; // seconds
   m_outputdir = cfg["outputDir"];
   m_timeDelayTolerance = cfg["timeDelayTolerance"]; // microseconds
   

 }

TLBMonitor::~TLBMonitor() { 
  INFO(__MODULEMETHOD_NAME__ << " With config: " << m_config.dump() << " getState: " << this->getState());
 }

void TLBMonitor::runner() {
  INFO(__MODULEMETHOD_NAME__ << " Running...");

  bool isData(true);

  m_error_rate_cnt = 0;
  seconds timestamp_in_seconds;
  bool update = true;
  auto starting_time_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
 
  while (m_run) {

    daqling::utilities::Binary eventBuilderBinary;

    isData = m_connections.get(1, eventBuilderBinary);
    if ( !isData ) std::this_thread::sleep_for(10ms);
    else {
        const EventHeader * eventHeader((EventHeader *)malloc(m_eventHeaderSize));
        EventFragmentHeader * fragmentHeader((EventFragmentHeader *)malloc(m_fragmentHeaderSize));
	bool dataOk = unpack_data( eventBuilderBinary, eventHeader, fragmentHeader );

	if (!dataOk) { 
		ERROR(__MODULEMETHOD_NAME__ << " ERROR in unpacking data "); 
		m_error_rate_cnt++;
		m_hist_map.fillHist("h_fragmenterrors","DataUnpack");
		continue;
	}

	uint32_t fragmentStatus = fragmentHeader->status;
	fill_error_status( "h_fragmenterrors", fragmentStatus );

        uint16_t payloadSize = fragmentHeader->payload_size; 

	m_hist_map.fillHist( "h_payloadsize", payloadSize);


        timestamp_in_seconds = duration_cast<seconds>(system_clock::now().time_since_epoch());
	if ( (timestamp_in_seconds.count() - starting_time_in_seconds)%m_timeblock == 0){
	    if ( update ) { 
	    std::cout<<" reached end of time block. current error rate is "<<m_error_rate_cnt<<std::endl;
	    m_hist_map.fillHist("g_error_rate_per_timeblock", std::make_pair(timestamp_in_seconds.count(), m_error_rate_cnt));
	    m_error_rate_cnt = 0;
	    update = false;
	    }
 	}
	else update = true;
    }

  }

  INFO(__MODULEMETHOD_NAME__ << " Runner stopped");

}

void TLBMonitor::initialize_hists() {

  INFO( __MODULEMETHOD_NAME__ << " ... initializing ... " );

  // TLB histograms

  RegularHist h_payloadsize = {"h_payloadsize","payload size [bytes]"};
  h_payloadsize.object = make_histogram(axis::regular<>(275, -0.5, 545.5, "payload size"));
  m_hist_map.addHist(h_payloadsize.name, h_payloadsize);

  CategoryHist h_fragmenterrors = { "h_fragmenterrors", "error type" };
  h_fragmenterrors.object = make_histogram(m_axis_fragmenterrors);
  m_hist_map.addHist(h_fragmenterrors.name, h_fragmenterrors);

  Graph g_error_rate_per_timeblock = {"g_error_rate_per_timeblock", "time [s]", "error rate" };
  m_hist_map.addHist(g_error_rate_per_timeblock.name, g_error_rate_per_timeblock);

  return ;

}
