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
    alias cmake=cmake3
fi

if [ ${USEAPT} -eq 1 ]
then 
    sudo apt-get update -y
    sudo apt-get install -y git cmake libcurl4-openssl-dev libjsoncpp-dev
    # Install CMAKE 3.13.4
    alias cmake=cmake3
    version=3.13
    build=4
    mkdir ~/temp
    cd ~/temp
    wget https://cmake.org/files/v$version/cmake-$version.$build.tar.gz
    tar -xzvf cmake-$version.$build.tar.gz
    cd cmake-$version.$build/
    ./bootstrap
    make -j8
    sudo make install
    cd ~
    rm -rf ~/temp
    cmake --version
fi

rm -rf drmlib
git clone --single-branch --branch dev https://github.com/Accelize/drmlib drmlib
cd drmlib/
mkdir build
cd build/
cmake ..
make -j8
sudo make install
cd ../..
