#ifndef EVENTFORMAT_H_
#define EVENTFORMAT_H_

struct TriggerMsg {
  int eventID;
  int bcID;
};

#define MAXFRAGSIZE 64000/4

struct Fragment {
  int source;
  int eventID;
  int bcID;
  int dataLength;
  int data[MAXFRAGSIZE];
  int headersize() { return 4; };
  int size() {
    return sizeof(int)*(headersize()+dataLength);
  }; 
};

#endif  /* EVENTFORMAT */
