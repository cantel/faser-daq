# Welcome to FASER/DAQ

Welcome! This documentation is intended to be your 
initial point of entry for getting involved in the [FASER experiment](https://faser.web.cern.ch/) TDAQ system.


## Software Layout
This software is based on the *DAQling* architecture
which orchestrates the running of individual
DAQ "processes".  In addition to the external DAQling 
architecture, a number of other FASER-specific software libraries are included that provide
hardware specific communication protocols (*digitizer-readout* and *gpiodrivers*) as well 
as tools shared between the online DAQ and offline reconstruction communities (*faser-common*).
All tools are included as [git submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) 
in the [faser/daq](https://gitlab.cern.ch/faser/daq) framework.  

### [DAQ-ling](https://gitlab.cern.ch/ep-dt-di/daq/daqling)
The DAQ-ling architecture is developed by the CERN EP-DT-DI group and is intended to serve
as a vanilla framework to flexibly execute processes that run concurrently and communicate
between each other.  Each process is a specific instance of a "module" and can either 
control and readout a piece of hardware (e.g. [DigitizerReceiverModule](https://gitlab.cern.ch/faser/daq/-/tree/master/src/Modules/DigitizerReceiver)) or aggregate and
process data entirely in software (e.g. [EventBuilder](https://gitlab.cern.ch/faser/daq/-/tree/master/src/Modules/EventBuilderFaser)).  The generic documentation for
DAQ-ling can be found here - [daqling.docs.cern.ch](https://daqling.docs.cern.ch/).

### [faser-common](https://gitlab.cern.ch/faser/faser-common)
The faser-common library is a central location where utilities that can be shared between
online and offline communities can be developed coherently.  This includes things 
such as the [EventFormat](https://gitlab.cern.ch/faser/faser-common/-/blob/master/EventFormats/EventFormats/DAQFormats.hpp) 
and hardware payload specific fragments and decoders (e.g. [DigitizerDataFragment](https://gitlab.cern.ch/faser/faser-common/-/blob/master/EventFormats/EventFormats/DigitizerDataFragment.hpp)). See the submodule README for a further description of faser-common utilities.

### [digitizer-readout](https://gitlab.cern.ch/faser/digitizer-readout)
The digitizer-readout library provides a library that can be used to control the sis3153+vx1730
pair of VME boards and retrieve data stored upon an acquisition trigger.  This relies on
the use of faser-common for the EventFormat and is used within the [DigitizerReceiverModule](https://gitlab.cern.ch/faser/daq/-/tree/master/src/Modules/DigitizerReceiver).
More extensive documentation of this library can be found in the submodule README.

### [gpiodrivers](https://gitlab.cern.ch/faser/gpiodrivers)
The GPIO Drivers library provides an interface for communication with the GPIO boards, which
control the TRB and the TLB.  It also provides specific functionality necessary for operation
of each of these boards.  More extensive documentation of this library can be found in the submodule README.

## Initial setup

This repository has several submodules, so it is easiest to clone with the `--recursive` option. NOTE that several compile options exist, described in the following section.

```
git clone --recursive <faser-daq git url>
cd faser-daq
source setup.sh
mkdir build; cd build
cmake ..
make
```

## CMake build options

The following CMake build options are available:

Flag | Options | Default | Description
---- | ------- | --------|  -----------
CMAKE_BUILD_TYPE | Release/Debug | Debug | Use `Debug` for testing, `Release` for production. Release adds optimization flags and so executes faster.
CMAKE_EMULATION_BUILD | ON/OFF | OFF | ON will only compile code relevant for emulation/playback mode. This should be used if you are not interacting with any detector components.

Use options like so

```
cmake .. -DCMAKE_EMULATION_BUILD=ON
``` 

## Running

FASER DAQ is a Finite State Machine. It is interactively controlled by sending state transitions such as `start`, `pause`, `stop` and `(shut) down`.
The FSM is booted up with a configuration file that provides the module configurations for start up.

There are two run modes for FASER DAQ: Via command line or via a Run Control web interface.

### Running from terminal
```
daqpy configs/<config_file_name>.json
```

### Running via the web interface

See the main instructions for the Run Control interface in the [README here](scripts/RunControl/README_How_To.md). To start the web server, you can execute one of two options

(a) Run Control without run service
```
rcguilocal
```

(b) Run Control with run service
```
rcgui
```

The latter connects the run control to the [FASER run database](https://faser-runinfo.app.cern.ch/), the run will be assigned a unique run number and all run information will be automatically logged in the database. Option (a) `rcguilocal` is preferred for debugging tests.

## Contacts/Experts
If you encounter an issue, do not hesitate to get in contact with someone.  The general
email list for the group is [FASER-tdaq@cern.ch](mailto:FASER-tdaq@cern.ch) to which you
can request subscription via the CERN e-group [https://e-groups.cern.ch/](https://e-groups.cern.ch/)
portal. Listed here are the specific
individuals who have particular expertise in a specific area and who you are recommended
to contact if you have issues :

  - Group Leaders/Organization : [Anna Sfyrla](mailto:Anna.Sfyrla@cern.ch), [Brian Petersen](mailto:Brian.Petersen@cern.ch)
  - DAQ-ling : [Enrico Gamberini](mailto:enrico.gamberini@cern.ch)
  - Digitizer : [Brian Petersen](mailto:Brian.Petersen@cern.ch)
  - TLB : [Claire Antel](mailto:claire.antel@cern.ch)
  - TRB : [Claire Antel](mailto:claire.antel@cern.ch), [Ondrej Theiner](mailto:ondrej.theiner@cern.ch)
  - gpiodrivers : [Claire Antel](mailto:claire.antel@cern.ch), [Kristof Schmieden](mailto:Kristof.Schmieden@cern.ch)
  - faser-common : [Brian Petersen](mailto:Brian.Petersen@cern.ch)
  
## Codebase
The entry-point for the code itself is stored on the CERN instance of GitLab at
[https://gitlab.cern.ch/faser/daq](https://gitlab.cern.ch/faser/daq).
A github mirror exists at [https://github.com/cantel/faser-daq](https://github.com/cantel/faser-daq) for external contributors.

This codebase is made public with the [GNU Lesser General Public License](https://www.gnu.org/licenses/lgpl-3.0.en.html).


















