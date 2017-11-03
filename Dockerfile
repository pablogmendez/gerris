FROM centos:latest

WORKDIR /

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig

# Add common directory
ADD Gerris /tmp/gerris

RUN yum update -y

########## Install dependencies ##########

# yum
RUN cp -R /tmp/gerris . && yum groupinstall "Development Tools" -y && \
	yum groupinstall "Compatibility Libraries" -y && yum install startup-notification-devel-0.12-8.el7.i686 startup-notification-devel-0.12-8.el7.x86_64 ncurses-devel zlib-devel texinfo gtk2-devel qt-devel tcl-devel tk-devel kernel-headers kernel-devel fftw-devel-3.3.3-8.el7.i686 fftw-devel-3.3.3-8.el7.x86_64 -y && \
	yum install gerris/packages/pangox-compat-0.0.2-2.el7.x86_64.rpm -y && yum install gerris/packages/pangox-compat-devel-0.0.2-2.el7.x86_64.rpm -y

# install gtklext
RUN tar -xvzf /gerris/packages/gtkglext-1.2.0.tar.gz && cd /gtkglext-1.2.0 && ./configure && make && make install

# install openmpi
RUN tar -xvzf /gerris/packages/openmpi-3.0.0.tar.gz && cd /openmpi-3.0.0 && ./configure && make && make install

# install hypre
RUN tar -xvzf /gerris/packages/hypre-2.11.2.tar.gz && cd /hypre-2.11.2/src && ./configure make && make install

# install ffmpeg
RUN tar -xvzf /gerris/packages/ffmpeg-3.4.tar.gz && cd ffmpeg-3.4 && ./configure --disable-x86asm && make && make install

########## Install Gerris ##########

# install gts
RUN cd /gerris/gerris/gts-stable && sh autogen.sh && automake --add-missing && ./configure && make && make install

# install gerris
RUN cd /gerris/gerris/gerris-stable && sh autogen.sh && automake --add-missing && touch test-driver && make && make install

# install Pablo gerris
RUN cd /gerris/Gerris-ControllerModule/gerris-stable && touch test-driver && sh autogen.sh && make && make install
