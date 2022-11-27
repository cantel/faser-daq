#
#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

from datetime import datetime
import requests


class MattermostNotifier:
    """
    Class allowing to send reminder alerts to Mattermost from a list of module names.
    By default, the message is "Module <name_of_module> has crashed". The reminder message is the same message but with "(reminder)" at the end.
    It is possible to set a new message template by specifying the string when creating a new MattermostNotifier object, where {} is the placeholder for the name of the module.
    """

    def __init__(self, mattermost_hook,  message_template="Module {} has crashed", time_interval = 60*60):

        self.__time_interval = time_interval  # sec
        self.__tracked = {}
        self.__message_template = message_template
        self.__mattermost_hook = mattermost_hook

    def check(self, modules):
        for module in modules:
            if module not in self.__tracked:
                self.__message(self.__message_template.format(module))
                self.__update_timestamp(module)
            elif module in self.__tracked:
                if (
                    int(datetime.now().timestamp()) - self.__tracked[module]
                    >= self.__time_interval
                ):
                    self.__message(self.__message_template.format(module) + " (reminder) ")
                    self.__update_timestamp(module)
        self.__clean_tracked(modules)

    def __clean_tracked(self, modules):
        """Cleans all the tracked modules that are no longer actives"""
        keys = list(self.__tracked.keys())
        for module in keys:
            if module not in modules:
                self.__tracked.pop(module)

    def __update_timestamp(self, modules):
        fired_timestamp = int(datetime.now().timestamp())
        self.__tracked[modules] = fired_timestamp
    
    def __message(self, msg):
        """
        Sends a message to mattermost. If there is an error, prints it.
        """
        if self.__mattermost_hook: 
            try:
                req = requests.post(self.__mattermost_hook,json={"text": msg})
                if req.status_code!=200:
                    print("Failed to post message below. Error code:", req.status_code)
                    print(msg)
            except Exception as e:
                print("Got exception when posting message",e)
        else:
            print(msg)