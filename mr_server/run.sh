#!/bin/bash


CNT_NAME=mt_reader
WORKDIR="$(pwd)"

pushd docker
docker build -t "$CNT_NAME" .
popd

docker run --rm -it \
    -v"${WORKDIR}":/root \
    -v /etc/localtime:/etc/localtime:ro \
    -p8085:8085 \
    "$CNT_NAME" \
    python /root/app.py


