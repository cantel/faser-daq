# daqling_top

Skeleton for project-specific DAQling Modules, scripts, and configurations.

## DAQling as a git submodule

    git submodule init
    git submodule update

## Host configuration and framework build

### Configure the CERN CentOS 7 host

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
    daqinterface configs/my-config.json
    start
    stop
    down

An example configuration (`valid-config.json`) and schema (`json-config.schema`) can be found in `daqling/configs/`. It is necessary to copy these files to the `configs/` folder and adapt them to describe the desired configuration.

`daqinterface -h` shows the help menu.

## Develop custom Modules

To develop a custom module, the existing modules in `daqling/src/Modules/` can provide guidance.

It is necessary to copy and rename the template folder `src/Modules/New` and its files to start developing the new module.

The custom module will be discovered and built by CMake as part of the project.

### Run custom Modules

To run a newly created Module (e.g. `MyNewModule`), it is necessary to add a corresponding entry in `components:` to a JSON configuration file. Note that the name of the Module needs to be specified in the `type:` field. E.g.:

    {
      "name": "mynewmodule01",
      "host": "localhost",
      "port": 5555,
      "type": "MyNewModule",
      "loglevel": {"core": "INFO", "module": "DEBUG"},
      "settings": {
      },
      "connections": {
      }    
    }
