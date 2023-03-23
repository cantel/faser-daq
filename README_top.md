# DAQling_top

Template project for custom DAQling Modules, scripts, and configurations.

## Documentation

Detailed documentation can be found at [https://daqling.docs.cern.ch](https://daqling.docs.cern.ch).

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

### Build dependencies installation

Refer to `daqling/README.md` for host setup instructions.

### Build

For the first-time sourcing of `daqling/cmake/setup.sh`, pass the location of the daqling-spack-repo to `daqling/cmake/setup.sh`:

    source daqling/cmake/setup.sh </full/path/to/daqling-spack-repo/>

More info in `README_SPACK.md`.

Then:

    source daqling/cmake/setup.sh
    mkdir build
    cd build
    cmake ..
    make

#### Advanced build options

Refer to `daqling/README.md` for advanced build instructions.

## Run a data acquisition system

`daqpy` is a command line tool that spawns and configures the components listed in the JSON configuration file passed as argument.

It then allows to control the components via standard commands such as `start` (with optional run number), `stop`, as well as custom commands.

    source daqling/cmake/setup.sh
    daqpy configs/demo.json
    start [run_num]
    stop
    down

An example configuration (`demo.json`) and schema (`schemas/validation-schema.json`) can be found in `daqling/configs/`. It is necessary to copy these files to the `configs/` folder and adapt them to describe the desired configuration.

`daqpy -h` shows the help menu.

## Develop custom Modules

To develop a custom module, the existing modules in `daqling/src/Modules/` can provide guidance.

It is necessary to copy and rename the template folder `src/Modules/New` and its files to start developing the new module.

The custom module will be discovered and built by CMake as part of the project.

### Run custom Modules

To run a newly created Module (e.g. `MyNewModule`), it is necessary to add a corresponding entry in `components:` to a JSON configuration file. Note that the name of the Module needs to be specified in the `type:` field. E.g.:

```json
{
  "name": "mynewmodule01",
  "host": "localhost",
  "port": 5555,
  "modules":[
   {
     "type": "MyNewModule",
     "name": "mynewmodule",
     "connections": {
     }    
   }
  ],
  "loglevel": {"core": "INFO", "module": "DEBUG","connection":"WARNING"},
  "settings": {
  }
}
```
