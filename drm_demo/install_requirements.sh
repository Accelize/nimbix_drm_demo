#!/bin/bash
! which yum > /dev/null 2>&1
USEYUM=$?
! which apt-get > /dev/null 2>&1
USEAPT=$?

if [ ${USEYUM} -eq 1 ]
then
	sudo yum install -y terminator geany
fi

if [ ${USEAPT} -eq 1 ]
then 
	sudo apt-get install -y terminator geany
fi
