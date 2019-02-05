#!/bin/bash
! which yum > /dev/null 2>&1
USEYUM=$?
! which apt-get > /dev/null 2>&1
USEAPT=$?

if [ ${USEYUM} -eq 1 ]
then
	sudo yum update -y
	sudo yum install -y epel-release
	sudo yum install -y git cmake3 libcurl-devel jsoncpp-devel ncurses ncurses-devel
fi

if [ ${USEAPT} -eq 1 ]
then 
	sudo apt-get update -y
	sudo apt-get install -y git cmake3 libcurl4-openssl-dev libjsoncpp-dev
fi

rm -rf drmlib
git clone --single-branch --branch dev https://github.com/Accelize/drmlib drmlib
cd drmlib/
mkdir build
cd build/
cmake3 ..
make -j8
sudo make install
cd ../..
