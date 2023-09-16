FROM python:latest

RUN pip install flask pillow webdavclient3 requests


COPY app.py /root/
COPY DejaVuSansMono.ttf /root/

WORKDIR /root
ENTRYPOINT python /root/app.py

VOLUME /root/photo_readings
VOLUME /root/firmware
