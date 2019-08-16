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


enum EventFragmentStatus { // to be reviewed as not all are relevant for FASER
  UnclassifiedError = 1,
  BCIDMismatch = 1<<1,
  TagMismatch = 1<<2,
  Timeout = 1<<3,
  Overflow = 1<<4,
  CorruptedFragment = 1<<5,
  DummyFragment = 1<<6,
  MissingFragment = 1<<7,  
  EmptyFragment = 1<<8,
  DuplicateFragment = 1<<9
};

enum EventMarkers {
  FragmentMarker = 0xAA,
  EventMarker = 0xBB
};

enum EventTags {
  PhysicsTag = 0x00,
  CalibrationTag = 0x01,
  MonitoringTag = 0x02,
  TLBMonitoringTag = 0x03
};

enum SourceIDs {
  TriggerSourceID = 0x020000,
  TrackerSourceID = 0x030000,
  PMTSourceID = 0x040000
};
  
const uint16_t EventFragmentVersion = 0x0001;
const uint16_t EventHeaderVersion = 0x0001;
  
struct EventFragmentHeader {
  uint8_t marker;
  uint8_t fragment_tag;
  uint16_t trigger_bits;
  uint16_t version_number;
  uint16_t header_size;
  uint32_t payload_size;
  uint32_t source_id;
  uint64_t event_id;   //could be reduced to 32 bits
  uint16_t bc_id;
  uint16_t status;
  uint64_t timestamp;
}  __attribute__((__packed__));

struct EventFragment {
  EventFragmentHeader header;
  uint32_t payload[sizeof(RawFragment)];
} __attribute__((__packed__));

struct EventHeader {
  uint8_t marker;
  uint8_t event_tag;
  uint16_t trigger_bits;
  uint16_t version_number;
  uint16_t header_size;
  uint32_t payload_size;
  uint8_t  fragment_count;
  unsigned int run_number : 24;
  uint64_t event_id;  //could be reduced to 32 bits
  uint16_t bc_id;
  uint16_t status;
  uint64_t timestamp;
}  __attribute__((__packed__));
