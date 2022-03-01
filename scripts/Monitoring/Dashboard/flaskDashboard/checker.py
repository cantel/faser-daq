#  Copyright (C) 2019-2020 CERN for the benefit of the FASER collaboration
#

class Checker():
    """
    Class that allows to identify basic problems on histograms (count_error, overflow). Has the flags : 
        - count_error 
        - uoverflow  

    To create a new flag :
        - Create a new function inside the class :
            def check_newFlag(self,data):
                ... logic goes here ...
                To be taken in account, the following must be appended in the self.flag array :
                    {"name" : <nameOfFlag>, "color": <theColorAssociatedWithFlag>}

        - Write the condition for which type of histograms the flag should be associated in the "check()" function
        
    """
    def __init__(self):
        self.flags = []

    def check_overflow(self, data):
        # The under/overflow are in the first and last bin of the histogram 
        if int(data[0]["y"][0]) != 0 or int(data[0]["y"][-1]) != 0 : 
            self.flags.append({"name":"uoverflow", "color": '#0a8bfc'})

    def check_countError(self, data):
        # checks if the sum is not zero -> if there is errors 
        if int(sum(data[0]["y"])) != 0:
            self.flags.append({"name":"count_error", "color": '#f4093c'})

    def check(self,histname, layout, data):
        if layout["hist_type"] == "categories" and "error" in histname :
            self.check_countError(data)
        elif layout["hist_type"] == "uoflow":
            self.check_overflow(data)



    
