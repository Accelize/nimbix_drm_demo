#!/bin/bash
cd /opt/accelize/drm_demo
SSHCMD="sudo ACCELIZE_DRM_VERBOSE=0 LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:/usr/local/lib ./helloworld_fpga -n 1 $*"

! xdpyinfo | grep VNC > /dev/null 2>&1
ISVNC=$?

# if running on VNC
if [ ${ISVNC} -eq 1 ]; then
	terminator -b -f -e "${SSHCMD}"
else
	${SSHCMD}
fi
