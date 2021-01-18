# Setup
The setup here pertains only to building the DAQ code and not configuring the hardware
properly (e.g. DCS).

## Obtaining the Code
The codebase is stored at [https://gitlab.cern.ch/faser/daq](https://gitlab.cern.ch/faser/daq)
and can be obtained by cloning it using any method that you prefer :
```
git clone ssh://git@gitlab.cern.ch:7999/faser/daq.git
```
Because there are a number of submodules that pull in other tools, these must also be
obtained by either using the `--recursive` option while cloning or performing an `init`/`update`
call from within the cloned `faser/daq` repository:
```
cd daq
git submodule init
git submodule update
```
which will populate the code within those submodules.  

## Building the Code
The build of faser/daq is configured with CMake.  If you are unfamiliar with this build
system, we recommend that you take an afternoon to familiarize yourself with the basics
by working through this [HEP Software Foundation tutorial on modern cmake](https://hsf-training.github.io/hsf-training-cmake-webpage/).
Little more than the concepts outlined here is used in our CMakeLists files.

### Dedicated Machines
A number of dedicated machines exist where we can build and test the software.  These
machines have the appropriate suite of software installed and are linked to the CERN
network such that you can log in with your normal CERN Computing Account credentials.

  - *faser-daq-001* [B21 SciLab] : This is connected to a standalone setup of a digitizer
  and TLB, but no TRB.  And there are not real faser scintillators or calorimeters hooked
  up to it.  However, there is a function generator and oscilloscope available for this setup.
  - *faser-daq-002* [ENH1] : This is connected to the commissioning hardware including digitizer,
  TLB, and TRB.  
  
All of these machines are within the CERN network meaning that they can only be *directly*
accessed if you computer is currently within the CERN network.  If you are at CERN, this 
is not a problem.  If you are not at CERN, then there are two solutions :

__[1]__ ssh to lxplus first and then ssh on to your desired machine

__[2]__ Tunnel all of your traffic through lxplus using this sshuttle command 
```
sshuttle --dns -v --remote lxplus-cloud.cern.ch 128.141.0.0/16 128.142.0.0/16 137.138.0.0/16 185.249.56.0/22 188.184.0.0/15 192.65.196.0/23 192.91.242.0/24 194.12.128.0/18
```
This should be run in a separate terminal in the background and after entering your 
you will be able to access the CERN-network-based computers from your machine.  Note that
this is also the manner by which you will be able to access the Run Control GUI (described later)
from outside the CERN network.

After logging onto one of these machines and obtaining the code as described above, you
can setup your build directory and build the code as :
```
cd daq
source setup.sh
mkdir build
cd build
cmake3 ../
make -j12
```
Note that you can perform a parallel build but it is wise to limit the number of cores
to no more than 12 on these machines.

### With Docker
Alternatively, if you don't have access to one of these machine but would like to build
the code on your own machine, this can be done by working within a [docker image](https://www.docker.com/).
If you are unfamiliar with docker, we recommend that you take an afternoon to familiarize yourself with the basics
by working through this [HEP Software Foundation tutorial on containerization](https://hsf-training.github.io/hsf-training-docker/index.html).
This is the system by which the continuous integration for our codebase is run and so knowing
the basics will benefit you if you run into issues there.

If you have docker installed and the daemon running on your machine, then you can spin
up a faser/daq container as follows :
```
cd daq
docker run --rm -it -v $PWD:$PWD gitlab-registry.cern.ch/faser/docker/daq:master
```
This will mount the current directory (e.g. `/home/path/to/faser/daq`) within the image
and so once you are within the image you can navigate to that location
```
/home/path/to/faser/daq
```
In this case, the setup requires one additional step of setting two environment variables 
to access the correct BOOST libraries in CMake, but otherwise the procedure to build
the code is the same :
```
source setup.sh
export BOOST_ROOT_DIR=/opt/lcg/Boost/1.70.0-eebf1/x86_64-centos7-gcc8-opt 
export BOOST_VERSION=1.70
mkdir build
cd build
cmake3 ../
make
```
Note that in this case, a parallel build is not recommended because you are working on your 
own machine and it probably isn't as powerful as the hardware in the labs.  You can try a
parallel build but it may cause your computer's fan to start working hard.

Finally, if you are using this method to build the code, you will not be able to use it to
perform a run since it is not hooked up to hardware (but perhaps this was obvious to you).

## Preparing a new FASER DAQ machine
To compile and run the FASER DAQ on a new machine (virtual or real), the machine first needs 
to be setup. For CentOS 7 machines at CERN, this follows the DAQling procedure with a few FASER
specific add-ons, `daqling/README.md`. Assuming the repository has been checked out, the
procedure to follow is as follows.  **Note** that you need SUDO rights on the machine on which
you are performing the installation.
```
#if needed setup proxy if there is no direct internet access, see below
#From DAQling
cd daqling
sudo yum install -y ansible
source cmake/setup.sh
cd ansible/
ansible-playbook set-up-host.yml --ask-become
ansible-playbook install-boost-1_70.yml --ask-become
ansible-playbook install-webdeps.yml --ask-become
ansible-playbook install-redis.yml --ask-become

#For FASER additional firewall ports need to be opened for the GPIO readout:
sudo firewall-cmd --zone=public --add-port=50000-50005/udp --permanent
sudo firewall-cmd --reload
#For FASER install additional python libraries for GUI:
sudo pip3 install requests
```
If running on machine without direct internet access, one has to setup a proxy on a different
machine and point `pip`, `yum` and `git` to it before running the above scripts. 
This can be done by adding the following to `/etc/pip.conf`:
```
[global]
http_proxy=http://faser-daq-001:8888
```
setting the environment variables in bash:
```
export http_proxy=http://faser-daq-001:8888
export https_proxy=http://faser-daq-001:8888
```
and setting `git` to use a proxy:
```
sudo git config --global http.proxy http://faser-daq-001:8888
```