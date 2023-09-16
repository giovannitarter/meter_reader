#!/bin/bash


IMG_NAME=mt_reader
WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

pushd mt_reader
docker build -t "$IMG_NAME" .
popd

docker run \
    --rm \
    -it \
    -v"$WORKDIR/photo_readings:/root/photo_readings" \
    -v"$WORKDIR/firmware:/root/firmware" \
    -v /etc/localtime:/etc/localtime:ro \
    -p 8085:8085 \
    --env-file "$WORKDIR/environment" \
    --entrypoint /root/app.py \
    "$IMG_NAME"

