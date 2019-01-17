#!/bin/bash
#!/bin/bash
! which yum > /dev/null 2>&1
USEYUM=$?
! which apt-get > /dev/null 2>&1
USEAPT=$?

if [ ${USEYUM} -eq 1 ]
then
	sudo yum update -y
	sudo yum install -y epel-release
	sudo yum install -y git gcc-c++ cmake libcurl-devel jsoncpp-devel boost boost-devel ncurses ncurses-devel
	git clone git://dotat.at/unifdef.git unifdef
	cd unifdef/
	make
	make install
	cd ..
fi

if [ ${USEAPT} -eq 1 ]
then 
	sudo apt-get update -y
	sudo apt-get install -y git gcc-c++ cmake libcurl4-openssl-dev libjsoncpp-dev boost unifdef
fi

git clone https://github.com/Accelize/drmlib drmlib
cd drmlib/
mkdir build
cd build/
cmake ..
make -j8
sudo make install
cd ../..
