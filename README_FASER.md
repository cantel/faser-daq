# FASER

This is the repository housing the top level software for the FASER DAQ. The primary
documentation for development and usage can be found here - [https://faserdaq.web.cern.ch/faserdaq/](https://faserdaq.web.cern.ch/faserdaq/)

## Installation

Follow instructions in the main README.md and daqling/README.md on how to clone the repository
and setup the host machine. Please note at moment you need:
  * To install at least the build directory on a non-AFS filesystem 
  * CentOS7 machine with root access (or a host that was already setup machine)
  * All the optional ansible options in daqling should be installed except Cassandra
  * The web gui also needs the following command to run
```
    sudo python3 -m pip install Flask_Scss
```

**NOTE** : When this installation documentation is inserted to the main documentation, it can
be deleted from here. 

## Documentation
The documentation which is available at [https://faserdaq.web.cern.ch/faserdaq/](https://faserdaq.web.cern.ch/faserdaq/)
is contained within the [`docs`](./docs) directory here and configured with the [mkdocs.yml](mkdocs.yml).  See 
the documentation for more details on usage.
