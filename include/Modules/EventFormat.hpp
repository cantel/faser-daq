#ifndef EVENTFORMAT_H_
#define EVENTFORMAT_H_

struct TriggerMsg {
  int event_id;
  int bc_id;
};

#define MAXFRAGSIZE 64000/4

struct RawFragment {  // will eventually be detector specific fragment...
  uint32_t event_id;
  uint32_t source_id;
  uint16_t bc_id;
  uint16_t dataLength;
  uint32_t data[MAXFRAGSIZE];
  int headerwords() { return 3; };
  int sizeBytes() {
    return sizeof(uint32_t)*(headerwords()+dataLength);
  }; 
}  __attribute__((__packed__));

enum EventFragmentStatus {
  CorruptedFragment = 0x1,
  EmptyFragment = 0x2,
  MissingFragment = 0x100,
  BCIDMismatch = 0x200
};

struct EventFragmentHeader {
  uint32_t event_id;
  uint32_t source_id;
  uint16_t bc_id;
  uint16_t payload_size;
  uint32_t status;
  uint64_t timestamp;
}  __attribute__((__packed__));

struct EventFragment {
  EventFragmentHeader header;
  char payload[];
} __attribute__((__packed__));

struct EventHeader {
  uint32_t event_id;
  uint16_t bc_id;
  uint16_t num_fragments;
  uint32_t status;
  uint64_t timestamp;
  uint32_t data_size;
}  __attribute__((__packed__));


#endif  /* EVENTFORMAT */
