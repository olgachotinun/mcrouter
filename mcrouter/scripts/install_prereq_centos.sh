#!/usr/bin/env bash

[ -n "$1" ] || ( echo "Install tmp dir is missing"; exit 1 )

cd $1

# install gcc 4.8 
sudo yum --enablerepo=devtoolset-2 install devtoolset-2-toolchain

# enable gcc 4.8:
scl enable devtoolset-2 bash

# intall all necessary packages
sudo yum install -y wget unzip gcc gcc-c++ make cmake libtool cyrus-sasl-lib cyrus-sasl-devel python-devel bison openssl-devel libevent-devel libcap-devel snappy snappy-devel binutils-devel scons flex krb5-devel numactl-devel

# install updated m4
wget -O m4-1.4.9.tar.gz http://ftp.gnu.org/gnu/m4/m4-1.4.9.tar.gz
tar -zvxf m4-1.4.9.tar.gz
cd m4-1.4.9
./configure
make
sudo make install
cd ..


# install updated autoconf
wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar xvfvz autoconf-2.69.tar.gz
cd autoconf-2.69
./configure
make
sudo make install
cd ..

# install Python 2.7
wget http://python.org/ftp/python/2.7.6/Python-2.7.6.tar.xz
tar xf Python-2.7.6.tar.xz
cd Python-2.7.6
./configure --prefix=/usr/local --enable-unicode=ucs4 --enable-shared \
LDFLAGS="-Wl,-rpath /usr/local/lib" && make 
sudo make altinstall
cd ..

# install boost
wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz/download -O boost_1_55_0.tar.gz
tar xvfvz boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh --prefix=/usr/local --libdir=/usr/local/lib --with-libraries=program_options,context,filesystem,system,regex,thread,python
## Boost always fails to update everything, just ignore it. Boost, you are the worst :)
set +e
sudo ./bjam -j4 --layout=system install
set -e
cd ..

# boost 1.55 does not have thread-mt and regex-mt needed for folly, so emulate them
sudo ln -s /usr/local/lib/libboost_thread.so.1.55.0 /usr/local/lib/libboost_thread-mt.so
sudo ln -s /usr/local/lib/libboost_regex.so.1.55.0 /usr/local/lib/libboost_regex-mt.so

# update ld config
sudo ldconfig
# not sure that this line is correct but otherwise thrift1 will not be able libboost*.so
echo /usr/local/lib | sudo tee --append /etc/ld.so.conf > /dev/null

# ragel
wget http://www.colm.net/wp-content/uploads/2014/10/ragel-6.9.tar.gz
tar -xvzf ragel-6.9.tar.gz 
cd ragel-6.9/
./configure
make
sudo make install
cd ..

# update lib config
sudo ldconfig

