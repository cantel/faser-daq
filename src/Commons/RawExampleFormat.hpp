#pragma once

#include <stdint.h>
#include "Commons/EventFormat.hpp"

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
  public:
    // getters
    uint32_t header() { return m_header; }
    uint32_t event_id() { return m_event_id; }
    uint32_t orbit_id() { return m_orbit_id; }
    uint32_t bc_id() { return m_bc_id; }
    uint32_t tbp0() { return m_tbp0; }
    uint32_t tbp1() { return m_tbp1; }
    uint32_t tbp2() { return m_tbp2; }
    uint32_t tbp3() { return m_tbp3; }
    uint32_t tbp4() { return m_tbp4; }
    uint32_t tbp5() { return m_tbp5; }
    uint32_t tap0() { return m_tap0; }
    uint32_t tap1() { return m_tap1; }
    uint32_t tap2() { return m_tap2; }
    uint32_t tap3() { return m_tap3; }
    uint32_t tap4() { return m_tap4; }
    uint32_t tap5() { return m_tap5; }
    uint32_t tav0() { return m_tav0; }
    uint32_t tav1() { return m_tav1; }
    uint32_t tav2() { return m_tav2; }
    uint32_t tav3() { return m_tav3; }
    uint32_t tav4() { return m_tav4; }
    uint32_t tav5() { return m_tav5; }
    uint32_t deadtime_veto_counter() { return m_deadtime_veto_counter; }
    uint32_t busy_veto_counter() { return m_busy_veto_counter; }
    uint32_t rate_limiter_veto_counter() { return m_rate_limiter_veto_counter; }
    uint32_t bcr_veto_counter() { return m_bcr_veto_counter; }

  private:
    uint32_t m_header;
    uint32_t m_event_id;
    uint32_t m_orbit_id;
    uint32_t m_bc_id;
    uint32_t m_tbp0;
    uint32_t m_tbp1;
    uint32_t m_tbp2;
    uint32_t m_tbp3;
    uint32_t m_tbp4;
    uint32_t m_tbp5;
    uint32_t m_tap0;
    uint32_t m_tap1;
    uint32_t m_tap2;
    uint32_t m_tap3;
    uint32_t m_tap4;
    uint32_t m_tap5;
    uint32_t m_tav0;
    uint32_t m_tav1;
    uint32_t m_tav2;
    uint32_t m_tav3;
    uint32_t m_tav4;
    uint32_t m_tav5;
    uint32_t m_deadtime_veto_counter;
    uint32_t m_busy_veto_counter;
    uint32_t m_rate_limiter_veto_counter;
    uint32_t m_bcr_veto_counter;
}  __attribute__((__packed__));

