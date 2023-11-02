FROM python:latest


ADD DejaVuSansMono.ttf /root/
ADD requirements.txt /root/

RUN pip install -r /root/requirements.txt
RUN pip freeze > /requirements.txt

ADD app.py /root/
WORKDIR /root
ENTRYPOINT python /root/app.py

VOLUME /root/photo_readings
VOLUME /root/firmware
