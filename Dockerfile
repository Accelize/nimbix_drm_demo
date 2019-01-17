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

# Create Build Workspace
RUN mkdir -p /opt/accelize_build
RUN chmod 777  /opt/accelize_build

# DRMLib Install
ADD drmlib_install.sh /opt/accelize_build/drmlib_install.sh
RUN chmod 777 /opt/accelize_build/drmlib_install.sh
RUN /opt/accelize_build/drmlib_install.sh

# Demo Copy and Compile
ADD drm_demo /opt/accelize_build/drm_demo
ADD drm_demo/drmdemo.desktop /home/nimbix/drmdemo.desktop 
ADD drm_demo/README.md /home/nimbix/README.md
RUN cd /opt/accelize_build/drm_demo; make clean all; sudo make install

# Remove Build Workspace
RUN rm -fr /opt/accelize_build

# Expose port 22 for local JARVICE emulation in docker
EXPOSE 22

# for standalone use
EXPOSE 5901
EXPOSE 443
