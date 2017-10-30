FROM centos:latest

WORKDIR /

ENV PKG_CONFIG_PATH /usr/local/lib/pkgconfig

# Add common directory
ADD Gerris /tmp/gerris

RUN echo "proxy=http://10.1.1.88:3128" >> /etc/yum.conf && \
	echo "proxy_username=pgmendez" >> /etc/yum.conf && \
	echo "proxy_password=Octubre2017" >> /etc/yum.conf && \
	yum update -y

# Install dependencies

RUN cp -R /tmp/gerris . && yum groupinstall "Development Tools" -y && \
	yum groupinstall "Compatibility Libraries" -y && yum install startup-notification-devel-0.12-8.el7.i686 startup-notification-devel-0.12-8.el7.x86_64 ncurses-devel zlib-devel texinfo gtk2-devel qt-devel tcl-devel tk-devel kernel-headers kernel-devel fftw-devel-3.3.3-8.el7.i686 fftw-devel-3.3.3-8.el7.x86_64 -y && \
	yum install gerris/packages/pangox-compat-0.0.2-2.el7.x86_64.rpm -y && yum install gerris/packages/pangox-compat-devel-0.0.2-2.el7.x86_64.rpm -y

RUN tar -xvzf gerris/packages/gtkglext-1.2.0.tar.gz && cd gtkglext-1.2.0 && ./configure && make && make install

# install gts

RUN cd /gerris/gerris/gts-stable && sh autogen.sh && automake --add-missing && ./configure && make && make install

# Add pkg-config-path

#RUN export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

# install gerris

RUN cd /gerris/gerris/gerris-stable && sh autogen.sh && automake --add-missing && make && make install

# install gfsview
RUN cd /gerris/gerris/gfsview-stable && sh autogen.sh CFLAGS='-lpthread -lX11' && automake --add-missing



#RUN chmod 777 /tmp/gerris/gerris.sh && /tmp/gerris/gerris.sh

#CMD tail -f /dev/null
