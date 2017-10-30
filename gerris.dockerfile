FROM centos:latest

WORKDIR /tmp

# Add common directory
ADD gerris /tmp/gerris

RUN echo "proxy=http://10.1.1.88:3128" >> /etc/yum.conf && \
	echo "proxy_username=pgmendez" >> /etc/yum.conf && \
	echo "proxy_password=Octubre2017" >> /etc/yum.conf && \
	yum update -y

RUN chmod 777 /tmp/gerris/gerris.sh && /tmp/gerris/gerris.sh

#CMD tail -f /dev/null