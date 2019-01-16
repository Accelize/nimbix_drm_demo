FROM nimbix/ubuntu-xrt:201802.2.1.83_16.04  
MAINTAINER Accelize, SAS.
# Xilinx Device Support Archive (DSA) for target FPGA
ENV DSA xilinx_u200_xdma_201820_1
# JARVICE Machine Type. nx5u: Alveo u200, nx6u: Alveo u250
ENV JARVICE_MACHINE nx5u
# FPGA bitstream to configure accelerator (*.xclbin format)
#ENV XCLBIN_PROGRAM drm_demo/bitstreams/u200/binary_container_1.xclbin
# FPGA bitstream to remove from container (*.xclbin format)
#ENV XCLBIN_REMOVE drm_demo/bitstreams/u200/binary_container_1.xclbin


# Metadata for App
ADD AppDef.json /etc/NAE/AppDef.json
RUN curl --fail -X POST -d @/etc/NAE/AppDef.json https://api.jarvice.com/jarvice/validate

# Create Accelize Workspace
#RUN mkdir -p /opt/accelize/

# DRMLib Install
ADD drmlib_install.sh /dev/shm/drmlib_install.sh
RUN chmod 777 /dev/shm/drmlib_install.sh
RUN /dev/shm/drmlib_install.sh

#ADD drmlib_install.sh /opt/accelize/drmlib_install.sh
#RUN chmod 777 /opt/accelize/drmlib_install.sh
#RUN /opt/accelize/drmlib_install.sh

# Demo Copy and Compile
ADD drm_demo /dev/shm/drm_demo
RUN cd /dev/shm/drm_demo; make clean all; sudo make install

#ADD drm_demo /opt/accelize/drm_demo/
#RUN chmod -R 777 /opt/accelize
#RUN make clean all -C /opt/accelize/drm_demo/

# Create alias for autorun
#RUN ln -s /opt/accelize/drm_demo/autorun.sh /usr/local/sbin/drmdemo

# Readme.md
#ADD README.md /opt/accelize/README.md

# Expose port 22 for local JARVICE emulation in docker
EXPOSE 22

# for standalone use
EXPOSE 5901
EXPOSE 443
