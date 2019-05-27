#!/usr/bin/env python3

import time
import sys

class BinaryReader:
    def __init__(self,name):
        self._fh=open(name,"rb")

    def readStruct(self,struct):
        vals=[]
        for ss in struct:
            size=int(ss)
            bytestring=self._fh.read(size)
            if len(bytestring)!=size:
                return None #reached EOF
            vals.append(int.from_bytes(bytestring,sys.byteorder))
        return vals

    def readWords(self,size):
        bytestring=self._fh.read(size*4)
        if len(bytestring)!=size*4:
            return None #reached EOF
        vals=[]
        for ii in range(size):
            vals.append(int.from_bytes(bytestring[ii*4:ii*4+3],sys.byteorder))
        return vals

class EventHeader:
    def __init__(self,rh):
        vals=rh.readStruct("422484")
        if vals:
            self.event_id=vals[0]
            self.bc_id=vals[1]
            self.num_fragments=vals[2]
            self.status=vals[3]
            self.timestamp=vals[4]
            self.data_size=vals[5]
        else:
            raise Exception("End of file")
    def __str__(self):
        return "Event %6d, BC %4d (%s) - status: 0x%08x, %2d fragments, %5d bytes" % (self.event_id,self.bc_id,time.ctime(self.timestamp/1000000),self.status,self.num_fragments,self.data_size)

class FragmentHeader:
    def __init__(self,rh):
        vals=rh.readStruct("442248")
        if vals:
            self.event_id=vals[0]
            self.source_id=vals[1]
            self.bc_id=vals[2]
            self.payload_size=vals[3]
            self.status=vals[4]
            self.timestamp=vals[5]
        else:
            raise Exception("End of file")
    def __str__(self):
        return "Fragment 0x%08x (Event %6d, BC %4d, %s) - status: 0x%08x, %5d bytes" % (self.source_id,self.event_id,self.bc_id,time.ctime(self.timestamp/1000000),self.status,self.payload_size)
   
def main(args):
    dumpFrag=False
    dumpData=False
    if args[0]=='-f' or args[0]=='-e':
        dumpFrag=True
        if args[0]=='-e': dumpData=True
        args=args[1:]
    rh=BinaryReader(args[0])
    
    try:
        while True:
            event=EventHeader(rh)
            print(event)
            if dumpFrag:
                for ii in range(event.num_fragments):
                    frag=FragmentHeader(rh)
                    print(" - "+str(frag))
                    data=rh.readWords(frag.payload_size//4)
                    if dumpData and data:
                        for ii in range(0,frag.payload_size//4,4):
                            hexData=["    %08x" % val for val in data[ii:ii+4]]
                            print("".join(hexData))
            else:
                rh.readWords(event.data_size//4)
    except:
        pass
    
main(sys.argv[1:])


                
            
