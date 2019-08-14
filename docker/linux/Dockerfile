FROM centos:6.10
RUN yum install wget -y
RUN wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
RUN yum install -y devtoolset-2-gcc devtoolset-2-binutils devtoolset-2-gcc-c++ mesa-libGLU-devel cmake git centos-release-scl && yum install -y python27 
RUN echo source /opt/rh/devtoolset-2/enable >> /root/.bash_profile
RUN scl enable python27 "/opt/rh/python27/root/usr/bin/easy_install-2.7 pip && /opt/rh/python27/root/usr/bin/pip2.7 install requests mkdocs"
