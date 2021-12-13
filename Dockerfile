FROM debian

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends --fix-missing\
    libgl-dev \
    qt5dxcb-plugin \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/

ADD https://download-test.robomongo.org/linux/robo3t-1.3.1-linux-x86_64-7419c406.tar.gz /opt/robo3t.tar.gz
RUN cd /opt/ \
    && mkdir robo3t \
    && tar -C /opt/robo3t --strip-components 1 -xzf robo3t.tar.gz && rm robo3t.tar.gz \
    && ls /opt/robo3t

VOLUME /root/.3T
VOLUME /root/.config/3T

RUN apt update && apt upgrade -y && apt autoremove -y
RUN apt install openssh-client -y

CMD /opt/robo3t/bin/robo3t
