#!/bin/bash
sudo ACCELIZE_DRM_VERBOSE=0 LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:/usr/local/lib ./helloworld_fpga $*
