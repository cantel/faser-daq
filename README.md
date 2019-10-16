# daqling_top

Skeleton for project-specific DAQling Modules and configurations.

## Clone DAQling as a git submodule

    git submodule init
    git submodule update

## Install the framework

### Configure your CentOS7 host

Refer to daqling/README.md for host setup instructions.

### Build

    source daqling/cmake/setup.sh
    mkdir build
    cd build
    cmake3 ..
    make

#### Advanced build options

Refer to `daqling/README.md` for advanced build instructions.

## Run
To run, you must be located in the `daq` directory which you cloned.  So if you are in the
build directory you just created, go up one level.

    source daqling/cmake/setup.sh
    daqinterface configs/your-config.json complete
    start
    stop
    down

You can find example `valid-config.json` and `json-config.schema` under `daqling/configs/`. You should copy these files to the `configs/` folder and adapt them to your DAQ needs.

`daqinterface -h` shows the help menu! 

## Develop custom Modules

In order to develop your own module, check the existing demonstration modules in `daqling/src/Modules` and `daqling/include/Modules` for guidance.

Copy and adapt the template `src/Modules/New/NewModule.cpp` and `include/Modules/New/NewModule.hpp` and start developing your custom module. 

Finally add the new custom module to `src/Modules/CMakeLists.txt` in order to build it as part of the project.

## For Novices

For those new to DAQ, described here is some guidance on the paradigm for how this framework operates.  
- Each application is a *Module* in this code base and when running, they will all run concurrently.
- Each *Module* has three main pieces :
   - 
