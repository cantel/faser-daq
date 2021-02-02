# !/usr/bin/python
#############################################
#
# Author : Sam Meehan
#
# Description : This script is meant to parse the contents of a directory and determine
#               if it contains the appropriate Copyright information for the FASER collaboration
#
#############################################
import sys
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-directory', type=str, required=True, help="Path to directory in which to search for copyright files")
parser.add_argument('-extensions', nargs='+', help='File extensions for the type of files that will be parsed for copyrights')
parser.add_argument('-exclude', nargs='+', help='File names to exclude from being parsed')

args = parser.parse_args()
directory  = args.directory
extensions = args.extensions
exclude    = args.exclude

print("Searching all files within : ",directory)

if extensions==None:
  print("No extensions to check")
  extensions=[]
  
if exclude==None:
  print("No files to explicitly be excluded from checking")
  exclude=[]

# extensions and exclusions to be used below
search_file_extensions = extensions
exclude_file_list      = exclude

# lists of offending files
offending_files_no_copyright=[]
offending_files_other_copyright=[]

# walk through the specified directory structure to check for copyrights
for root, dirs, files in os.walk(directory, topdown=False):

   # only inspect paths if they are files
   for name in files:
   
      filepath = os.path.join(root, name)
   
      # ensure that we inspect what we want to inspect
      if len(filepath.split("/"))>1:
        if len(filepath.split("/")[-1].split("."))>1:
          file_extension = filepath.split("/")[-1].split(".")[-1]
      
          if file_extension in search_file_extensions:
            if filepath.split("/")[-1] not in exclude_file_list:
              print("Searching file for Copyright : ",filepath)
            
              # do they have any copyright
              has_copyright = False
              for line in open(filepath, "r").readlines():
                if "copyright" in line or "Copyright" in line:
                  has_copyright = True
                
              # if they have a copyright, do they have our copyright
              faser_copyright = "Copyright (C) 2020 CERN for the benefit of the FASER collaboration"
              has_faser_copyright = False
              lines = open(filepath, "r").readlines()
              for line in open(filepath, "r").readlines():
                if ("Copyright (C)" in line) and ("CERN for the benefit of the FASER collaboration" in line):
                  has_faser_copyright = True
                
              # classify
              if has_copyright and not has_faser_copyright:
                offending_files_other_copyright.append(os.path.join(root, name))
              elif not has_copyright:
                offending_files_no_copyright.append(os.path.join(root, name))
   
   
if len(offending_files_other_copyright)==0 and len(offending_files_no_copyright)==0:
  print("")
  print("=======================================================")
  print("All files that should be copyrighted appear to be copyrighted")
  exit(0)
else:
  print("")
  print("=======================================================")
  print("The following files have no copyright : ")
  for file in offending_files_no_copyright:
    print(" >>> ",file)
  if len(offending_files_no_copyright)==0:
    print(" >>> NO FILES FOUND <<<")

  print("")
  print("=======================================================")
  print("The following files have a copyright that is not consistent with the FASER copyright :")
  for file in offending_files_other_copyright:
    print(" >>> ",file)   
    
  print("")
  print("It may not necessarily be the case that there is an issue")
  print("but if you know that one of these files should actually not")
  print("have a copyright, then please add it to the list of files")
  print("to ignore using the [-exclude] option. Please see the [-help]")
  print("menu for usage.")
  print("")
  exit(1)
   
   
   
   
   
   
   
   
   
   
