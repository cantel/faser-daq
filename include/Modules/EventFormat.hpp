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
  int headersize() { return 3; };
  int size() {
    return sizeof(int)*(headersize()+dataLength);
  }; 
}  __attribute__((__packed__));

struct EventFragmentHeader {
  uint32_t event_id;
  uint32_t source_id;
  uint16_t bc_id;
  uint16_t payload_size; 
  uint64_t timestamp;
}  __attribute__((__packed__));

struct EventFragment {
  EventFragmentHeader header;
  char payload[];
} __attribute__((__packed__));

#endif  /* EVENTFORMAT */
