#!/bin/bash
cd /opt/accelize/drm_demo
DRMDEMO_SSHCMD="sudo ACCELIZE_DRMLIB_VERBOSE=0 LD_LIBRARY_PATH=/opt/xilinx/xrt/lib:/usr/local/lib ./drm_demo -n 1 $*"
DRMDEMO_VNCCMD="terminator -b -f -e ${DRMDEMO_SSHCMD}"

! xdpyinfo | grep VNC > /dev/null 2>&1
ISVNC=$?

# if running on VNC
if [ ${ISVNC} -eq 1 ]; then
	terminator -b -f -e "${DRMDEMO_SSHCMD}"
else
	${DRMDEMO_SSHCMD}
fi
