#!/bin/bash
git clone https://github.com/Accelize/drmlib
cd drmlib/
mkdir build
cd build/
sudo apt-get update
sudo apt-get install -y libcurl4-openssl-dev libjsoncpp-dev unifdef
cmake ..
make -j8
sudo make install
cd ../..
