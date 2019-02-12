#!/bin/bash
cd /opt/accelize/helloworld_fpga
SSHCMD="./run.sh -n 1 $*"

! xdpyinfo | grep VNC > /dev/null 2>&1
ISVNC=$?

# if running on VNC
if [ ${ISVNC} -eq 1 ]; then
	terminator -b -f -e "${SSHCMD}"
else
	${SSHCMD}
fi
