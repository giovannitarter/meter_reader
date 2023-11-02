#!/bin/bash


WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

pushd docker
docker build -t espcam_builder .
#docker build --no-cache -t espcam_builder .
popd

rm -rf "$WORKDIR/output"
mkdir -p "$WORKDIR/output"
docker run \
    --rm \
    -it \
    -v"$WORKDIR/output":/output \
    -v"$WORKDIR/project":/root/project \
    --device "/dev/ttyUSB0" \
    espcam_builder \
    sh -c 'pio run -t upload && pio device monitor'

