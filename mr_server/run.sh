#!/bin/bash


CNT_NAME=mt_reader
WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

pushd docker
docker build -t "$CNT_NAME" .
popd

docker run \
    --rm \
    -it \
    -v"$WORKDIR":/root \
    -v /etc/localtime:/etc/localtime:ro \
    -p 8085:8085 \
    --env-file "$WORKDIR/.environ" \
    "$CNT_NAME" \
    python /root/app.py

