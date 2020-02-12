#pragma once

#include <stdint.h>

struct TriggerMsg {
  int event_id;
  int bc_id;
};

#define MAXFRAGSIZE 64000/4

enum RawTypes {
  dataType = 0x01,
  monType = 0x02
};


struct RawFragment {  // will eventually be detector specific fragment...
  uint32_t type;
  uint32_t event_id;
  uint32_t source_id;
  uint16_t bc_id;
  uint16_t dataLength;
  uint32_t data[MAXFRAGSIZE];
  int headerwords() { return 4; };
  int sizeBytes() {
    return sizeof(uint32_t)*headerwords()+dataLength;
  }; 
}  __attribute__((__packed__));

struct MonitoringFragment {  // will eventually be detector specific fragment...
  uint32_t type;
  uint32_t counter;
  uint32_t source_id;
  uint32_t num_fragments_sent;
  uint32_t size_fragments_sent;
}  __attribute__((__packed__));

struct TLBMonitoringFragment { 
  uint32_t header;
  uint32_t event_id;
  uint32_t orbit_id;
  uint32_t bc_id;
  uint32_t tap0;
  uint32_t tap1;
  uint32_t tap2;
  uint32_t tap3;
  uint32_t tap4;
  uint32_t tap5;
  uint32_t tav0;
  uint32_t tav1;
  uint32_t tav2;
  uint32_t tav3;
  uint32_t tav4;
  uint32_t tav5;
  uint32_t deadtime_veto_counter;
  uint32_t busy_veto_counter;
  uint32_t rate_limiter_veto_counter;
  uint32_t bcr_veto_counter;
}  __attribute__((__packed__));

