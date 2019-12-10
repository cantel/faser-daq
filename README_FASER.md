# FASER

FASER specific instructions

## Installation

Follow instructions in the main README.md and daqling/README.md on how to clone the repository
and setup the host machine. Please note at moment you need:
  * To install at least the build directory on a non-AFS filesystem 
  * CentOS7 machine with root access (or a host that was already setup machine)
  * All the optional ansible options in daqling should be installed except Cassandra
  * The web gui also needs the following command to run

    sudo python3 -m pip install Flask_Scss

## Setup

To setup command paths etc. in a fresh shell, one should run the following everytime (it assumes bash shell)

    source setup.sh

It is not necessary to source daqling/cmake/setup.sh as that is done in the above script.

## Build

See commands in main README.md (skip the setup script)

## Run 

One can use the command line interface (See README.md) or the web-based GUI. 
The GUI is started with
    rcgui

Connect to port 5000 on host in a webbrowser. More detailed instructions can be found in:
https://espace.cern.ch/faser-share/Shared%20Documents/Trigger,%20DAQ%20and%20DCS/FASER_Run_Control_User_s_Guide.pdf

