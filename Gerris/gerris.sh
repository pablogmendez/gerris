#!/bin/bash
cd /
mkdir gerris
cp -R /tmp/gerris/Gerris/* /gerris

echo "proxy=http://10.1.1.88:3128" >> /etc/yum.conf
echo "proxy_username=pgmendez" >> /etc/yum.conf
echo "proxy_password=Octubre2017" >> /etc/yum.conf
yum update -y

yum groupinstall "Development Tools" -y
yum groupinstall "Compatibility Libraries" -y
yum install startup-notification-devel-0.12-8.el7.i686 startup-notification-devel-0.12-8.el7.x86_64 ncurses-devel zlib-devel texinfo gtk2-devel qt-devel tcl-devel tk-devel kernel-headers kernel-devel fftw-devel-3.3.3-8.el7.i686 fftw-devel-3.3.3-8.el7.x86_64 -y
yum install gerris/packages/pangox-compat-0.0.2-2.el7.x86_64.rpm -y
yum install gerris/packages/pangox-compat-devel-0.0.2-2.el7.x86_64.rpm -y

tar -xvzf gerris/packages/gtkglext-1.2.0.tar.gz
cd gtkglext-1.2.0
./configure
make && make install

tar -xvzf gerris/packages/openmpi-3.0.0.tar.gz 
cd openmpi-3.0.0
./configure
make && make install

tar -xvzf gerris/packages/hypre-2.11.2.tar.gz
cd hypre-2.11.2/src
./configure
make && make install

tar -xvzf /gerris/packages/ffmpeg-3.4.tar.gz
cd ffmpeg-3.4
./configure --disable-x86asm
make && make install

cd /gerris/gerris/gts-stable
sh autogen.sh && automake --add-missing
./configure
make && make install

export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

cd /gerris/gerris/gerris-stable
sh autogen.sh && automake --add-missing
make && make install

#cd /gerris/gerris/gfsview-stable
#sh autogen.sh CFLAGS='-lpthread -lX11' && automake --add-missing
#make && make install