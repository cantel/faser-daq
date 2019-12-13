#!/usr/bin/env python3

import time
import sys

class BinaryReader:
    def __init__(self,name):
        self._fh=open(name,"rb")

    def readStruct(self,struct):
        vals=[]
        #print(struct)
        #print(type(struct))
        for ss in struct:
            #print("next : ",ss)
            #print(type(ss))
            size=int(ss)
            bytestring=self._fh.read(size) # gets "size" bytes from the file
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
        vals=rh.readStruct("1122241388228")
        if vals:
            self.marker=vals[0]
            self.event_tag=vals[1]
            self.trigger_bits=vals[2]
            self.version_number=vals[3]
            self.header_size=vals[4]
            self.payload_size=vals[5]
            self.fragment_count=vals[6]
            self.run_number=vals[7]
            self.event_id=vals[8]
            self.event_counter=vals[9]
            self.bc_id=vals[10]
            self.status=vals[11]
            self.timestamp=vals[12]
        else:
            raise Exception("End of file")
        if self.marker!=0xBB:
            raise Exception("Wrong Event Marker")
        if self.version_number!=0x0001:
            raise Exception("Wrong Event Format Version")
        if self.header_size!=4*11:
            raise Exception("Wrong header size")
    def __str__(self):
        return "Run %6d, Event %6d, BC %4d (%s) - status: 0x%08x, tag: %2d, trigger: 0x%04x, %2d fragments, %5d bytes" % (self.run_number,self.event_id,self.bc_id,time.ctime(self.timestamp/1000000),self.status,self.event_tag,self.trigger_bits,self.fragment_count,self.payload_size)

class FragmentHeader:
    def __init__(self,rh):
        vals=rh.readStruct("11222448228")
        if vals:
            self.marker=vals[0]
            self.event_tag=vals[1]
            self.trigger_bits=vals[2]
            self.version_number=vals[3]
            self.header_size=vals[4]
            self.payload_size=vals[5]
            self.source_id=vals[6]
            self.event_id=vals[7]
            self.bc_id=vals[8]
            self.status=vals[9]
            self.timestamp=vals[10]
        else:
            raise Exception("End of file")
        if self.marker!=0xAA:
            raise Exception("Wrong Fragment Marker")
        if self.version_number!=0x0001:
            raise Exception("Wrong Event Format Version")
        if self.header_size!=4*9:
            raise Exception("Wrong header size")
    def __str__(self):
        return "Fragment 0x%08x (Event %6d, BC %4d, %s) - status: 0x%08x, tag: %2d, trigger: 0x%04x, %5d bytes" % (self.source_id,self.event_id,self.bc_id,time.ctime(self.timestamp/1000000),self.status,self.event_tag,self.trigger_bits,self.payload_size)
   
def main(args):

    print("Args : ",args)

    dumpFrag=False
    dumpData=False
    if args[0]=='-f' or args[0]=='-e':
        dumpFrag=True
        if args[0]=='-e': dumpData=True
        args=args[1:]
    print("args[0] : ",args[0])
    rh=BinaryReader(args[0])
    
    try:
        while True:
            event=EventHeader(rh)
            print(event)
            
            #x=input()
            
            if dumpFrag:
                for ii in range(event.fragment_count):
                    frag=FragmentHeader(rh)
                    print(" - "+str(frag))
                    data=rh.readWords(frag.payload_size//4)
                    if dumpData and data:
                        for ii in range(0,frag.payload_size//4,4):
                            hexData=["    %08x" % val for val in data[ii:ii+4]]
                            print("".join(hexData))
            else:
                rh.readWords(event.payload_size//4)
    except Exception as inst:
        if str(inst)!="End of file":
            print(inst)
    

main(sys.argv[1:])


                
            
