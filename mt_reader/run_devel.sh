#!/bin/bash


IMG_NAME=mt_reader
WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

pushd mt_reader
docker build $@ -t "$IMG_NAME" .
RES_BUILD="$?"
popd

if [ $RES_BUILD != 0 ];
then
    echo "Image build error"
    exit 1
fi 

if [ ! -e environment ];
then
    cp environment.sample environment
fi

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

