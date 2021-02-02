# Monitoring
While you are performing a run, in addition to the output data being written to disk, 
there are also means by which to monitor the state of the system.  These monitoring
capabilities allow you to inspect what your software is doing as well as the quality
of the data being produced.  This is done on the fly, thereby allowing interventions if 
necessary.  


## Log Files
The first means by which to monitor the operation of the system is through the log
files produced - one for each module that is included within a run. These can help inspect what is
going on at a single given point in time in a detailed manner but should not be used to
give a birds eye view of operation.  They are much better at providing necessary debugging capabilities.  Each line in these log
files show :

  - __Timestamp__ : Date and time of the execution of this log message
  - __Source__ : Whether the message came from the `module` in which is is implemented or from the external `core` libraries
  - __Severity__ : The level of severity, which itself can be toggled in the configuration to show more or less information for debugging purposes
  - __Location__ : The source code and line at which the log message is located
  - __Function__ : The function in which the log message resides
  - __Message__ : The actual specific log message itself

<details><summary>Click to see an example snippet from a log file of the Digitizer</summary>
<p>
```
[2020-10-21 16:03:30.038] [core] [info] [Core.cpp:30] [bool daqling::core::Core::setupCommandPath()]   BINDING COMMAND SOCKET : tcp://*:5560
[2020-10-21 16:03:30.040] [core] [info] [ConnectionManager.cpp:43] [bool daqling::core::ConnectionManager::setupCommandConnection(uint8_t, std::__cxx11::string)]   Command is connected on: tcp://*:5560
[2020-10-21 16:03:30.163] [core] [info] [ConnectionManager.cpp:135] [bool daqling::core::ConnectionManager::addChannel(unsigned int, daqling::core::ConnectionManager::EDirection, const string&, size_t, unsigned int, size_t)]   Adding SERVER channel for: [0] bind: tcp://*:8104
[2020-10-21 16:03:30.217] [module] [info] [DigitizerReceiverModule.cpp:23] [DigitizerReceiverModule::DigitizerReceiverModule()]  
[2020-10-21 16:03:30.217] [module] [info] [DigitizerReceiverModule.cpp:24] [DigitizerReceiverModule::DigitizerReceiverModule()]  DigitizerReceiverModule Constructor
[2020-10-21 16:03:30.218] [module] [info] [DigitizerReceiverModule.cpp:30] [DigitizerReceiverModule::DigitizerReceiverModule()]  Getting IP Address
[2020-10-21 16:03:30.219] [module] [info] [DigitizerReceiverModule.cpp:48] [DigitizerReceiverModule::DigitizerReceiverModule()]  Input locale : 128.141.151.198
[2020-10-21 16:03:30.219] [module] [info] [Helper.cpp:333] [std::__cxx11::string GetIPAddress(char*)]  Interpreting as direct IP address
[2020-10-21 16:03:30.219] [module] [info] [Helper.cpp:360] [std::__cxx11::string GetIPAddress(char*)]  Discovered digitizer IP address : 128.141.151.198
[2020-10-21 16:03:30.219] [module] [info] [DigitizerReceiverModule.cpp:51] [DigitizerReceiverModule::DigitizerReceiverModule()]  Input address/host  : "128.141.151.198"
[2020-10-21 16:03:30.219] [module] [info] [DigitizerReceiverModule.cpp:52] [DigitizerReceiverModule::DigitizerReceiverModule()]  Returned IP Address : 128.141.151.198
[2020-10-21 16:03:30.219] [module] [info] [DigitizerReceiverModule.cpp:66] [DigitizerReceiverModule::DigitizerReceiverModule()]  Digitizer IP Address : 128.141.151.198
[2020-10-21 16:03:30.219] [module] [info] [DigitizerReceiverModule.cpp:70] [DigitizerReceiverModule::DigitizerReceiverModule()]  Getting VME digitizer HW address
[2020-10-21 16:03:30.220] [module] [info] [DigitizerReceiverModule.cpp:80] [DigitizerReceiverModule::DigitizerReceiverModule()]  Base VME Address = 0x32000000
[2020-10-21 16:03:30.220] [module] [info] [Comm_vx1730.cpp:36] [vx1730::vx1730(char*, unsigned int)]  vx1730::Constructor
[2020-10-21 16:03:30.220] [module] [info] [Comm_vx1730.cpp:54] [vx1730::vx1730(char*, unsigned int)]  Openning VME connection : 
[2020-10-21 16:03:30.220] [module] [info] [Comm_vx1730.cpp:55] [vx1730::vx1730(char*, unsigned int)]  get_vmeopen_messages = sis3153eth UDP port is open
[2020-10-21 16:03:30.220] [module] [info] [Comm_vx1730.cpp:56] [vx1730::vx1730(char*, unsigned int)]  nof_found_devices    =                    1
[2020-10-21 16:03:30.221] [module] [info] [Helper_sis3153.cpp:131] [int sis3153_TestComm(sis3153eth*, bool)]  sis3153_TestComm
[2020-10-21 16:03:30.222] [module] [debug] [Helper_sis3153.cpp:162] [int sis3153_TestComm(sis3153eth*, bool)]  =====================================
[2020-10-21 16:03:30.222] [module] [debug] [Helper_sis3153.cpp:163] [int sis3153_TestComm(sis3153eth*, bool)]  Testing interface card communications with LED A on and off
[2020-10-21 16:03:30.223] [module] [debug] [Helper_sis3153.cpp:164] [int sis3153_TestComm(sis3153eth*, bool)]  =====================================
[2020-10-21 16:03:30.223] [module] [debug] [Helper_sis3153.cpp:165] [int sis3153_TestComm(sis3153eth*, bool)]  Turn LED-A on
[2020-10-21 16:03:30.323] [module] [debug] [Helper_sis3153.cpp:170] [int sis3153_TestComm(sis3153eth*, bool)]  Turn LED-A off
[2020-10-21 16:03:30.424] [module] [debug] [Helper_sis3153.cpp:175] [int sis3153_TestComm(sis3153eth*, bool)]  Turn LED-A on
[2020-10-21 16:03:30.424] [module] [debug] [Helper_sis3153.cpp:179] [int sis3153_TestComm(sis3153eth*, bool)]  The light should still be on for LED A on the interface card
[2020-10-21 16:03:30.427] [module] [info] [Comm_vx1730.cpp:80] [void vx1730::TestCommDigitizer(json)]  Testing read/write to scratch space on digitizer
[2020-10-21 16:03:30.428] [module] [info] [Comm_vx1730.cpp:96] [void vx1730::TestCommDigitizer(json)]  Check Fixed Configurations
[2020-10-21 16:03:30.429] [module] [info] [Comm_vx1730.cpp:110] [void vx1730::TestCommDigitizer(json)]  VX1730_CONFIG_ROM : 24
[2020-10-21 16:03:30.429] [module] [info] [Comm_vx1730.cpp:111] [void vx1730::TestCommDigitizer(json)]  VX1730_CONFIG_ROM_BOARD_VERSION : 196
[2020-10-21 16:03:30.429] [module] [info] [Comm_vx1730.cpp:112] [void vx1730::TestCommDigitizer(json)]  VX1730_CONFIG_ROM_BOARD_FORMFACTOR : 1
[2020-10-21 16:03:30.429] [module] [info] [Comm_vx1730.cpp:113] [void vx1730::TestCommDigitizer(json)]  VX1730_ROC_FPGA_FW_REV : 1108870166
[2020-10-21 16:03:30.430] [module] [info] [Comm_vx1730.cpp:115] [void vx1730::TestCommDigitizer(json)]  BoardType config : "v0"
[2020-10-21 16:03:30.430] [module] [info] [Comm_vx1730.cpp:117] [void vx1730::TestCommDigitizer(json)]  Validating board type : v0
[2020-10-21 16:03:30.430] [module] [warning] [Comm_vx1730.cpp:119] [void vx1730::TestCommDigitizer(json)]  VX1730_CONFIG_ROM : This is not consistent with board v0 specs.
[2020-10-21 16:03:30.430] [module] [warning] [Comm_vx1730.cpp:121] [void vx1730::TestCommDigitizer(json)]  VX1730_CONFIG_ROM_BOARD_VERSION : This is not consistent with board v0 specs.
[2020-10-21 16:03:30.430] [module] [warning] [Comm_vx1730.cpp:125] [void vx1730::TestCommDigitizer(json)]  VX1730_ROC_FPGA_FW_REV : This is not consistent with board v0 specs.

```
</p>
</details>
<br>

In the source code, the amount of log messages that will be printed is intrinsically 
linked to the severity with which you choose to perform a run for a specific module.
The four levels of increasing severity that can be selected for a given module correspond to :

  - `DEBUG`  : These message should be implemented to allow for the inspection of very detailed information within the code.
  - `INFO`   : These message should be used to see standard information.
  - `WARNING`: These message should alert the user that something funky is going on, but will not break the system.
  - `ERROR`  : These message should alert the user that the system is about to break to has broken.  

Precisely which level of severity is printed for a given module can be controlled within the configuration
of that module by choosing the levels at this line for the module itself or the module calls to the core code.

```
"loglevel": {"core": "INFO", "module": "DEBUG"},
```

## Run Monitoring
In addition to the log files, there are two complementary monitoring applications that
provide the ability to inspect the operation of the system from a broader position and 
more quickly diagnose problems.  However, unlike the log files, they do not provide
detailed information connected to the source code that is running.  These two applications
are :

  - Time-based Metrics : Used for inspection of single valued metrics ("scalars") and their
  evolution over the course of time during running.
  - Histograms : Used for inspection of aggregated data from the past.

### Time-based Metrics
The first monitoring application pertains to the inspection of "scalars" stored in a time
sequence.  This time sequence is saved in a [Redis](https://redis.io/) database and subsequently queried
by the [Grafana](https://grafana.com/) application to provide a user-friendly view of the data.  

![](img/example_grafana.png)

Accessing this dashboard was already covered in the [section on running](./running). The Grafana application itself is well-documented by the industry sources elsewhere
and we encourage users to seek that documentation and implement it at will.  For example, in this web-GUI
any individual can modify the *Dashboard* to do the following :

  - Monitor a new published metric stored in Redis : 
  - Include additional calculated metrics or : 
  - Configure the dashboard to send automated warnings to emails : 
  
As you can see, the documentation for doing many things is plentiful and so we forego 
re-writing this here. If you do find a useful
feature that you think would be helpful for other FASER collaborators to know about, consider 
adding it to this documentation by following the steps [outlined here](./documentation) and
including it in the list above or creating a subsection below.

#### Your New Grafana Monitoring Feature
Have a new monitoring feature that you want others to know about? Add it here.

### Histograms

__CLAIRE OR EDWARD__






















