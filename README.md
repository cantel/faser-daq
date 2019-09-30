# daqling_top

Skeleton for project-specific DAQling Modules, scripts and configurations.

## Clone DAQling as a git submodule

    git submodule init
    git submodule update

## Install the framework

### Configure your CentOS 7 host

Refer to daqling/README.md for host setup instructions.

### Build

    source daqling/cmake/setup.sh
    mkdir build
    cd build
    cmake3 ..
    make

#### Advanced build options

Refer to daqling/README.md for advanced build instructions.

## Run

    source daqling/cmake/setup.sh
    daqinterface configs/your-config.json
    start
    stop
    down

You can find example configuration (`valid-config.json`) and schema (`json-config.schema`) in `daqling/configs/`. You should copy these files to the `configs/` folder and adapt them to your DAQ needs.

`daqinterface -h` shows the help menu!

## Develop custom Modules

In order to develop your own module, check the existing demo modules in `src/Modules/New/` for guidance.

Copy and adapt the template folder `src/Modules/New` and develop your custom module.

The custom module will be discovered (don't forget to modify the name of the module in the `CMakeLists.txt` file) and built by CMake as part of the project.
