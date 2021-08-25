
# TrackerCalibration

This package contains the code required to perform the calibration of the FASER tracker.

Note that it contains the trackerreadout package as a submodule.


## Installation
```
git clone --recursive ssh://git@gitlab.cern.ch:7999/faser/online/trackercalibration.git
cd trackercalibration
source gpiodrivers/setup.sh
mkdir build
cd build
cmake3 ../
make
```

## Running `tcalib`
`tcalib` is the main binary that allows to perform the different scans needed
for the calibration of the SCT modules. After compilation it gets created in `build/bin`. For a list of available options type `tcalib -h`:

```
Usage: ./tcalib -i <JSON_FILE> -t <TEST_IDENTIFIER> [OPTION(s)] 

where : 
  <JSON_FILE>        Input json configuration file (full plane or single module)
  <TEST_IDENTIFIER>  is a number corresponding to one or more of the tests listed below:
     1 : L1delay scan    
     2 : Mask scan       
     3 : Threshold scan  
     4 : Strobe delay    
     5 : 3-point gain    
     6 : Trim scan       
     7 : Response curve  
     8 : Noise occupancy
     9 : Trigger burst

Optional arguments: 
   -o, --outBaseDir <OUTDIR>       Base output results directory (default: /home/shifter/cernbox/b161).
   -v, --verbose <VERBOSE LEVEL>   Sets the verbosity level from 0 to 3 included (default=0).
   -d, --l1delay <L1DELAY>         delay between calibration-pulse and L1A (default: 130)
   -l, --log <LOG_FILE>            Output logfile, relative to outBaseDir (default: log.txt).
   -e, --emulate                   Emulate TRB interface (default=false).
   -c, --calLoop                   Use calLoop functionality (default=false).
   -n, --noTrim                    Do not load in chips the single-channel trimDac values (default=false).
   -u, --usb                       Use TRB connected via USB (default=false).
       --ip <IPADDRESS>            SCIP / DAQIP address for ethernet communication (default: 10.11.65.6).
   -h, --help                      Displays this information and exit.
```

## Running threshold-scans: `thscan`

The `thscan` binary allows to perform threshold-scans, with additional
command-line options such as the number of triggers, the input charge or the readout-mode, in
addition to the standard options from `tcalib` (see above). For a list of available options type `thscan -h`:

```
Usage: ./thscan -i <JSON_FILE> [OPTION(s)] 

where : 
  <JSON_FILE>  Input json configuration file (full plane or single module)

Optional arguments: 
   -o, --outBaseDir <OUTDIR>       Base output results directory (default: ~/run).
   -v, --verbose <VERBOSE LEVEL>   Sets the verbosity level from 0 to 3 included (default: 0).
   -d, --l1delay <L1DELAY>         delay between calibration-pulse and L1A (default: 130)
   -q, --charge <CHARGE>           Input calibration charge in fC (default: 1 fC)
   -r, --readout-mode <MODE>       Readout mode for ABCD: 0=HIT (1XX, X1X or XX1), 1=LEVEL (X1X), 2=EDGE (01X), 3=Test (XXX) (default: 1)
   -t, --trig <NTRIGGERS>          Number of triggers at each step (default=100)
   -l, --log <LOG_FILE>            Output logfile, relative to outBaseDir (default: log.txt).
   -c, --calLoop                   Use calLoop functionality (default=false).
   -n, --noTrim                    Do not load in chips the single-channel trimDac values (default=false).
   -h, --help                      Displays this information and exit.
```


