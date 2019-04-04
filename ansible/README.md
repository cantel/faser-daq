# Install the rd51 framework

## Set up your host and install supervisord

### Install ansible on CentOS7

    sudo yum install -y ansible

### Run ansible-playbook to configure your host

    cd ansible/
    source daq-ansible.sh
    cd playbooks/
    ansible-playbook set-up-host.yml  --ask-become-pass

