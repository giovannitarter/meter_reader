#!/bin/bash


WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

if [ $1 = "--no-cache" ];
then
    CACHE="--no-cache"
    shift
else
    CACHE=""
fi


pushd docker
docker build $CACHE -t espcam_builder .
popd

DEVICE="/dev/ttyUSB0"

if [ $# = 0 ];
then
    if [ -e "$DEVICE" ];
    then
        CMD='pio run -t upload && pio device monitor' 
        DEVICE="--device /dev/ttyUSB0"
    else
        CMD='pio run'
        DEVICE=""
    fi
else 
    DEVICE=""
    CMD="$@"
fi

#rm -rf "$WORKDIR/output"
#mkdir -p "$WORKDIR/output"

echo ""
echo "spawning container"
echo "$CMD"
echo ""

set -x

docker run \
    --rm \
    -it \
    -v"$WORKDIR/../mt_reader/firmware":/output \
    -v"$WORKDIR/project":/root/project \
    $DEVICE \
    espcam_builder \
    sh -c "$CMD"
    
    #sh -c 'pio run -t upload && pio device monitor'

