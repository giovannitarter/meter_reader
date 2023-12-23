FROM ubuntu:latest


ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install apt-utils
RUN apt-get -y install curl python3 python3-serial python3-venv
RUN curl -fsSL \
    https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
    -o get-platformio.py
RUN python3 get-platformio.py

ENV PATH=/root/.platformio/penv/bin:$PATH
RUN pio pkg install --global --platform espressif32
RUN pio pkg install --global --tool platformio/tool-mkspiffs
RUN pio pkg install --global --tool tool-mklittlefs
RUN pio pkg install --global --tool tool-mkfatfs
RUN pio pkg install --global --tool framework-arduinoespressif32@3.20011.230801
RUN pio pkg install --global --tool tool-scons@4.40502.0


RUN mkdir -p "/root/project"
VOLUME ["/root/project"]

RUN mkdir -p "/output"
#VOLUME ["/output"]

#COPY entrypoint.sh /
#COPY create_manifest.py /root/
#ENTRYPOINT /entrypoint.sh

ENV DEPLOY_DIR="/output"

WORKDIR /root/project
