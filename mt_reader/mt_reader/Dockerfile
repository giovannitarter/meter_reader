FROM python:latest


COPY app.py /root/
COPY DejaVuSansMono.ttf /root/
COPY requirements.txt /root/

RUN pip install -r /root/requirements.txt
RUN pip freeze > /requirements.txt

WORKDIR /root
ENTRYPOINT python /root/app.py

VOLUME /root/photo_readings
VOLUME /root/firmware
