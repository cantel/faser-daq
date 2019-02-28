# RD51-daq

DAQ framework for RD51

# Install the rd51 framework
### Run ansible-playbook to configure your CentOS7 host
The playbook will set up your host with the system libraries and will install supervisord

    sudo yum install -y ansible
    cd ansible/
    ansible-playbook set-up-host.yml --ask-become-pass

## Software compilation

    source cmake/setup-cvmfs.sh
    git submodule init
    git submodule update
    mkdir build
    cd build
    cmake3 ../
    make

You can also do incremental compilation like:

    make utilities
    make core

### Running

TBA

```
pwd
```

## How to add a submodule
### Cereal

    git submodule add https://github.com/USCiLab/cereal.git vendor/cereal --single branch

    cd vendor/cereal
    git checkout v1.2.2
    git add
    git commit
¬
###Spdlog

    git submodule add https://github.com/gabime/spdlog.git vendor/spdlog --single branch

    cd vendor/spdlog
    git checkout 1.3.1
    git add
    git commit

### Docopt

    git submodule add https://github.com/docopt/docopt.cpp.git vendor/docopt --single branch

    cd vendor/docopt
    git checkout v0.6.2
    cd ../
    git add
    git commit

### Json

    git submodule add https://github.com/nlohmann/json.git vendor/json --single branch

    cd vendor/json
    git checkout v3.5.0
    cd ../
    git add
    git commit

