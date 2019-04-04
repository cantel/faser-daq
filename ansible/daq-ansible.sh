# daq-ansible.sh
# This file meant to be sourced
# Set ansible environment variables to run from local directory instead
# of /etc/ansible.
# 
ansible_home=`pwd`
export ANSIBLE_CONFIG=$ansible_home/ansible.cfg
export ANSIBLE_INVENTORY=$ansible_home/hosts
export ANSIBLE_LIBRARY=$ansible_home
