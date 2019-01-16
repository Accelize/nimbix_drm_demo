#!/bin/bash

terminator -f --working-directory="/opt/accelize/drm_demo/" -x sudo ACCELIZE_DRMLIB_VERBOSE=0 LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:/usr/local/lib ./drm_demo -u mary
