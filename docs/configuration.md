# Configuration 
The configuration input for performing a run is fully encapsulated in a [json](https://www.json.org/) formatted
text file.  This text file specifies effectively a list of modules, each of which will
be executed by DAQ-ling as a separate process running concurrently.  For this set of 
modules, two primary things are contained within the configuration file :

  - __System Specific Configurations__ : These are the specific configuration inputs that
  are needed to guide the particular module that is to be run.  For example, nn the case of the
  `DigitizerReceiverModule`, one such configuration would be the fixed trigger threshold
  above which a hardware trigger signal is sent to the TLB.  All of these can be found for
  each module within the `settings` entry of a given modules configuration.
  - __Intermodule Connections__ : These are the network ports that specify how information
  is passed between modules.  They come in two flavors, `senders` and `receivers`, which
  as their name implies govern what information is sent from a module to downstream modules and what information 
  is picked up as dynamic input (as opposed to configuration input) from upstream modules.
  
Described here is a pragmatic view of how to go about reading or writing these connections
in the case of FASER.  A more generic overview of the DAQ-ling approach can be found in 
the [native DAQ-ling documentation](https://daqling.docs.cern.ch/).

For each of the subsystems here, the various portions of the `json` configuration are
described.  This information is parsed from the associated [schema](https://json-schema.org/) for that module, so
if data is missing, then it can and should, be improved by augmenting the schema appropriately.
The general format for the settings show below are as

  - `config_entry` (*Title on Seen in RCGui*) : 
    - __Description__ : Description of what the configuration parameter does. If no description is present then this will say MISSING and someone should improve the schema.
    - __NItems__ : If the entry is an array then this will show you the minimum and maximum number of entries that are expected.
    - __Default__ : The default value, if any.
    - __Possible Values__ : If there are a limited set of choices, this will display the possible values from which you can choose
    - __Minimum Value__ : If the entry has bounds, the minimum value that it can take.
    - __Maximum Value__ : If the entry has bounds, the maximum value that it can take.


## Trigger Logic Board Settings
The TLB is responsible for distributing L1 accept signals to the TRB and the digitizer
for acquiring data.  This can be done either by processing the digitizer LVDS signals
through the Look-Up-Table and prescales, or by generating triggers internally.  The 
more commonly used TLB configurations can be found in the 
[the TLB template](https://gitlab.cern.ch/faser/daq/-/blob/master/configs/Templates/TLB.json)

{%
   include-markdown "TriggerReceiver.md"
%}

<details><summary>Click to see an example TLB configuration file</summary>
<p>
{%
   include-markdown "TLBReceiverEHN1.md"
%}
</p>
</details>
<br>

## Digitizer Settings
The digitizer is responsible for providing the LVDS input signals to the TLB that it uses
to ascertain whether an L1 accept is necessary.  The digitizer also acquires the scintillator
and calorimeter signals via an ADC.  With these features in mind, the primary configuration
parameters have to do with the trigger threshold and logic settings as well as the ADC 
settings for acquisition window width and placement. The more commonly used digitizer 
configurations can be found in the  [the Digitizer template](https://gitlab.cern.ch/faser/daq/-/blob/master/configs/Templates/Digitizer.json).

{%
   include-markdown "DigitizerReceiver.md"
%}

<details><summary>Click to see an example Digitizer configuration file</summary>
<p>
{%
   include-markdown "DigitizerEHN1.md"
%}
</p>
</details>
<br>

## Tracker Receiver Board
The TRB is responsible for acquiring hits in the silicon strip trackers and buffering this data until
a L1 accept from the TLB.  The configs for this subsystem are largely stored in external files because of
their size.  Examples of the TRB configurations in their native format can be found by examining 
[the TLB template](https://gitlab.cern.ch/faser/daq/-/blob/master/configs/Templates/TRB.json).

{%
   include-markdown "TrackerReceiver.md"
%}

<details><summary>Click to see an example TRB configuration file</summary>
<p>
{%
   include-markdown "TRBReceiver00.md"
%}
</p>
</details>
<br>

## Event Builder
The EventBuilder is responsible for assembling the event fragments in a combined payload 
and adding an event header before sending it to the FileWriter for writing out.  Because this
is purely software and no hardware needs to be configured, there are relatively few settings
that primarily pertain to verifying whether the construction of the event is going well or
there is some error.  Examples of the EventBuilder configurations in their native format can be found by 
examining [the EventBuilderFaser template](https://gitlab.cern.ch/faser/daq/-/blob/master/configs/Templates/eventBuilder.json).

{%
   include-markdown "EventBuilderFaser.md"
%}

<details><summary>Click to see an example EventBuilder configuration file</summary>
<p>
{%
   include-markdown "EventBuilder.md"
%}
</p>
</details>
<br>

## File Writer
The FileWriter receives the event from the EventBuilder and does nothing more than writing
this to an output stream to disk.  As such, there are relatively few settings - only those
which control where the data goes and the maximum size of each output file.
Examples of the FileWriter configurations in their native format can be found by 
examining [the FileWriterFaser template](https://gitlab.cern.ch/faser/daq/-/blob/master/configs/Templates/fileWriter.json).

{%
   include-markdown "FileWriterFaser.md"
%}

<details><summary>Click to see an example WileWriter configuration file</summary>
<p>
{%
   include-markdown "FileWriter.md"
%}
</p>
</details>
<br>

### Working with Schemas
In order to use the GUI to edit settings for a ny givencomponent, a json-schema
has to be provided in "configs/<moduleName>.schema". Only the `name`,`type` and `settings`
schema has to be provided. A first iteration of the
schema can be generated by pasting in the relevant json configuration into:
  https://www.liquid-technologies.com/online-json-to-schema-converter
However, hand-editing is necessary to add allowed ranges for values,
ordering of the items in the display etc. Details can be found at:
  https://github.com/json-editor/json-editor/tree/master
and tried out at:
  https://pmk65.github.io/jedemov2/dist/demo.html
(we use the "tailwind" framework with "grid" layout")

One can check the schema without starting the gui by running:
  ./json-validate.py <jsonFile.json> [ModuleType]
  
Furthermore, a very nice online GUI editor to create and modify your starting
json schema file to get something useable is the [https://github.com/rjsf-team/react-jsonschema-form](https://github.com/rjsf-team/react-jsonschema-form)
which subsequently points you to [https://rjsf-team.github.io/react-jsonschema-form/](https://rjsf-team.github.io/react-jsonschema-form/). This 
one is also pretty good [https://github.com/jdorn/json-editor](https://github.com/jdorn/json-editor) which has an
online editor as well as [https://www.jeremydorn.com/json-editor](https://www.jeremydorn.com/json-editor).


## Monitoring Modules

### Digitizer

### Trigger

### Tracker

### EventBuilder

## Intermodule Connections
In addition to the module-specific configuration settings, it is necessary to tie the 
entire system together to enable data to flow between modules.  This is achieved by using
ports configured as `receivers` and `senders` for each module.  This is describe in greater
detail in the [relevant section of the DAQ-ling documentation](https://daqling.docs.cern.ch/modules/)
but in short, what you need in your configuration is something like this :
```
{
  "name": "Digitizer"
  ...
  "connections": {
    "senders": [
      {
        "type": "pubsub",
        "chid": 0,
        "transport": "tcp",
        "host": "*",
        "port": 8101
      }
    ]
  }
},
{
  "name": "EventBuilder"
  ...
  "connections": {
    "receivers": [
      {
        "type": "pair",
        "chid": 0,
        "transport": "tcp",
        "host": "localhost",
        "port": 8101
      }
    ]
  }
}
```
This will allow data to be pushed out by the `Digitizer` module and picked up by the
`EventBuilder` module because they are both configured to communicate on port 8101.
As you can see, the `senders` and `receivers` are both arrays, and there can be multiple
instances for each module, identified by their unique `chid`.

__NOTE__ : This is often one of the most common issues when constructing configuration
files, so be careful that you are tying your system together appropriately.

