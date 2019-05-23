# daqling_top
Skeleton for project-specific DAQling Modules and configurations.

# Clone DAQling as a git submodule:

    git submodule init
    git submodule update

Update to the latest DAQling release with:
    
    cd daqling/
    git pull

# Install the framework
## Configure your CentOS7 host
Refer to daqling/README.md for host setup instructions.

## Build

    source daqling/cmake/setup.sh
    mkdir build
    cmake3 ..
    make

### Advanced build options
Refer to daqling/README.md for advanced build instructions.

# Run

    source daqling/cmake/setup.sh
    daqinterface configs/your-config.json complete
    start
    stop
    down

You can find example `valid-config.json` and `json-config.schema` under `daqling/configs/`. You should copy these files to the `configs/` folder and adapt them to your DAQ needs.

`daqinterface -h` shows the help menu! 

# Develop custom Modules
In order to develop your own module, check the existing demonstration modules in `daqling/src/Modules` and `daqling/include/Modules` for guidance.

Copy and adapt the `src/Modules/NewModule.cpp_template` and `include/Modules/NewModule.hpp_template` and start developing your custom module.

Finally add the new custom module to `src/Modules/CMakeLists.txt` in order to build it as part of the project.

