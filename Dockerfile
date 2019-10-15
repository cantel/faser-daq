FROM atlas/centos7-atlasos:latest

RUN cd /tmp/ && \
 wget https://centos7.iuscommunity.org/ius-release.rpm && \
 rpm -i --nodeps ius-release.rpm && \
 yum remove -y git && \
 yum install -y git2u && \
 yum clean all
 
RUN yum install -y libusbx-devel && \
 yum clean all
 
ENV HOME /home
 
COPY startup.sh $HOME/startup.sh
 
ENTRYPOINT ["/bin/bash", "/home/startup.sh","/bin/bash"] 


