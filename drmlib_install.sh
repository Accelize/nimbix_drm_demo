#!/bin/bash
! which yum > /dev/null 2>&1
USEYUM=$?
! which apt-get > /dev/null 2>&1
USEAPT=$?

if [ ${USEYUM} -eq 1 ]
then
    sudo yum install -y epel-release
    sudo yum install -y git libcurl-devel jsoncpp-devel ncurses ncurses-devel
    sudo yum remove -y cmake
fi

if [ ${USEAPT} -eq 1 ]
then 
    sudo apt-get update -y
    sudo apt-get install -y git libcurl4-openssl-dev libjsoncpp-dev
    sudo apt remove --purge --auto-remove cmake
fi

sudo pip3 install cmake
rm -rf drmlib
git clone --single-branch --branch dev https://github.com/Accelize/drmlib drmlib
git checkout 164904d8f2cab765ee78004bd543d328be4d1d9d
cd drmlib/
mkdir build
cd build/
cmake ..
make -j8
sudo make install
cd ../..
