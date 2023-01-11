#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

from datetime import datetime
import requests
import socket

class MattermostNotifier:
    """
    Class allowing to send reminder alerts to Mattermost from a list of module names.
    By default, the message is "Module <name_of_module> has crashed". The reminder message is the same message but with "(reminder)" at the end.
    It is possible to set a new message template by specifying the string when creating a new MattermostNotifier object, where {} is the placeholder for the name of the module.
    """

    def __init__(self, mattermost_hook,  message_template="Module {} has crashed", time_interval = 60*60, okAlerts = True):

        self.__time_interval = time_interval  # sec
        self.__tracked = {}
        self.__message_template = message_template
        self.__mattermost_hook = mattermost_hook
        self.__okAlerts = okAlerts

    def check(self, modules):
        for module in modules:
            if module not in self.__tracked:
                self.__message(self.__message_template.format(module), module)
                self.__update_timestamp(module)
            elif module in self.__tracked:
                if (
                    int(datetime.now().timestamp()) - self.__tracked[module]
                    >= self.__time_interval
                ):
                    self.__message(self.__message_template.format(module) + " (reminder) ",module)
                    self.__update_timestamp(module)
        self.__clean_tracked(modules)

    def __clean_tracked(self, modules):
        """Cleans all the tracked modules that are no longer actives"""
        keys = list(self.__tracked.keys())
        okModules = []
        for module in keys:
            if module not in modules:
                self.__tracked.pop(module)
                okModules.append(module)
        if self.__okAlerts :
            if len(okModules) != 0:
                a = "\n * " # backslashes are not directly supported in f-strings
                msg = f":white_check_mark: The following modules are in __OK__ status :\n * {a.join(okModules)}"
                self.__message(msg,okStatus=True)

    def __update_timestamp(self, modules):
        fired_timestamp = int(datetime.now().timestamp())
        self.__tracked[modules] = fired_timestamp
    
    def __message(self, msg:str, module = None, okStatus=False):
        """
        Sends a message to mattermost. If there is an error, prints it.
        """
        if not okStatus :
            hostname = socket.gethostname() 
            additionalInfo = f"\n * Link to RCGUI : [http://{hostname}.cern.ch:5000](http://{hostname}.cern.ch:5000/)\n * Link to the module's live log: [here](http://{hostname}:9001/logtail/faser:{module})"
            msg += additionalInfo
        if self.__mattermost_hook: 
            try:
                req = requests.post(self.__mattermost_hook,json={"text": msg, "channel": "faser-ops-alerts", "username":"RCGUI-alerts"})
                if req.status_code!=200:
                    print("Failed to post message below. Error code:", req.status_code)
                    print(msg)
            except Exception as e:
                print("Got exception when posting message",e)
        else:
            print(msg)
