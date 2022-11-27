## How to use the Run Control GUI
To be able to perform an action on the run control gui, you must be logged in using your CERN account. 

Note : 
* Sometimes, the login action doesn't work well and a page called "Invalid State" appears. 
The easiest way is to go back to the main page by deleting everything after the base url. 
It is also possible to click on the link on the error page. 


Once logged in, to avoid that two people perform actions at the same time, you must claim interlock. To do this, click on the lock at the top right. Your username will then be displayed next to it. 

To change the configuration, click on the button, which will list all available configurations. (If a configuration does not appear, check that the folder with the same name exists)

To see the logs of a module, click on its name in the tree view. A "LOG" button will appear and it will be possible to choose between "full log" and "live log". "Full log" allows to see all the log, but it will not be updated in real time. The "live log" will show only the last lines, but will be in real time. Clicking on the name will also display additional information about the module in a separate panel, in real time. 

In the "CONTROL" panel, it is possible to initialise, start, stop, pause, shutdown and ECR.

Notes:
* At `START` a window will pop up in which you need to:
    * Write message describing reason for this run.
    * Select the run type (from "Cosmics", "Test", "Calibration", "Physics")
* At `STOP` a window will pop up in which you need to: 
    * Write message describing the reason for stopping this run.
    * At this point, you can choose to change the run type.
*  START/STOP messages and run type are logged in the [RunList](https://faser-runinfo.app.cern.ch/).
*  Do not go from `PAUSE` to `STOP` state.
    *  Instead, do `PAUSE`->`START`->`STOP`->`SHUTDOWN`, in order to stop a run.
    *  If this is not done, the run must be force shutdown.

![ScreenCapture](https://gitlab.cern.ch/faser/online/faser-daq/-/blob/new-rcgui/scripts/RunControl/img/interface.png)


    