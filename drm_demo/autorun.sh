#!/bin/bash
cd /opt/accelize/drm_demo
terminator -b -f -e "sudo ACCELIZE_DRMLIB_VERBOSE=0 LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:/usr/local/lib ./drm_demo -n 1 $*"

