FROM nimbix/ubuntu-xrt:201802.2.1.83_16.04  
MAINTAINER Accelize, SAS.
# Xilinx Device Support Archive (DSA) for target FPGA
ENV DSA xilinx_u200_xdma_201820_1
# JARVICE Machine Type. nx5u: Alveo u200, nx6u: Alveo u250
ENV JARVICE_MACHINE nx5u

# Metadata for App
ADD AppDef.json /etc/NAE/AppDef.json
ADD screenshot.png /etc/NAE/screenshot.png
RUN curl --fail -X POST -d @/etc/NAE/AppDef.json https://api.jarvice.com/jarvice/validate

# Install Dependencies
RUN apt-get -y update
RUN apt-get -y install gedit geany terminator

# Create Build Workspace
RUN mkdir -p /opt/accelize_build
RUN chmod 777  /opt/accelize_build

# DRMLib Install
ADD drmlib_install.sh /opt/accelize_build/drmlib_install.sh
RUN chmod 777 /opt/accelize_build/drmlib_install.sh
RUN /opt/accelize_build/drmlib_install.sh

# Demo Copy and Compile
ADD helloworld_fpga /opt/accelize_build/helloworld_fpga
ADD rc.local /opt/accelize_build/rc.local
RUN rm -f /etc/rc.local; mv /opt/accelize_build/rc.local /etc/rc.local; chmod +x /etc/rc.local
RUN cd /opt/accelize_build/helloworld_fpga; make clean all; sudo make install

# Change SSH Welcome Banner
RUN rm -f /etc/update-motd.d/*; cp -f /opt/accelize/helloworld_fpga/ssh_welcome.txt /etc/motd

# Remove Build Workspace
RUN rm -fr /opt/accelize_build

# Expose port 22 for local JARVICE emulation in docker
EXPOSE 22

# for standalone use
EXPOSE 5901
EXPOSE 443
