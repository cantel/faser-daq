# DAQling_top

Template project for custom DAQling Modules, scripts, and configurations.

## Documentation

Detailed documentation can be found [here][codimd].

[codimd]: <https://codimd.web.cern.ch/s/B1oArin-r>

Subscribe to the "daqling-users" CERN e-group for updates.

To contact the developers: daqling-developers@cern.ch (only for "daqling-users" members).

## DAQling as a git submodule

After cloning DAQling_top, the following commands are necessary to clone DAQling as a submodule:

    git submodule init
    git submodule update

After every `git checkout` or `git pull`

    git submodule update

will checkout the correct DAQling version for that commit.

## Host configuration and framework build

### Configure the CERN CentOS 7 host

Refer to `daqling/README.md` for host setup instructions.

### Build

    source daqling/cmake/setup.sh
    mkdir build
    cd build
    cmake3 ..
    make

#### Advanced build options

Refer to `daqling/README.md` for advanced build instructions.

## Run a data acquisition system
To run, you must be located in the `daq` directory which you cloned.  So if you are in the
build directory you just created, go up one level.  After ensuring you are in the proper
directory, you can use the `daqinterface` command.  This is a command line tool that 
spawns and configures the components listed in the JSON configuration file passed as argument.

It then allows to control the components via standard commands such as `start` 
(with optional run number), `stop`, as well as custom commands.

    source daqling/cmake/setup.sh
    daqinterface configs/my-config.json
    start [run_num]
    stop
    down

An example configuration (`valid-config.json`) and schema (`json-config.schema`) can be found in `daqling/configs/`. It is necessary to copy these files to the `configs/` folder and adapt them to describe the desired configuration.

`daqinterface -h` shows the help menu.

## Develop custom Modules

To develop a custom module, the existing modules in `daqling/src/Modules/` can provide guidance.

It is necessary to copy and rename the template folder `src/Modules/New` and its files to start developing the new module.

The custom module will be discovered and built by CMake as part of the project.

### Run custom Modules

<<<<<<< HEAD
Copy and adapt the template `src/Modules/New/NewModule.cpp` and `include/Modules/New/NewModule.hpp` and start developing your custom module. 

Finally add the new custom module to `src/Modules/CMakeLists.txt` in order to build it as part of the project.

## For Novices

For those new to DAQ, described here is some guidance on the paradigm for how this framework operates.  
- Each application is a *Module* in this code base and when running, they will all run concurrently.
- Each *Module* has three main pieces :
   - 
=======
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
>>>>>>> master
