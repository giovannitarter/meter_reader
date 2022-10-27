#!/bin/bash


if [ -z "$STASSID" ];
then
    echo "missing STASSID in ENV"
    exit 1
fi

if [ -z "$STAPSK" ];
then
    echo "missing STAPSK in ENV"
    exit 1
fi

if [ -z "$CVERSION" ];
then
    echo "missing CVERSION in ENV"
    exit 1
fi

if [ -z "$SRVNAME" ];
then
    SRVNAME="espcamsrv1"
fi

if [ -z "$SRVPORT" ];
then
    SRVPORT="8085"
fi

cd /root/build
export PLATFORMIO_BUILD_FLAGS=" \
-DSTASSID=\\\"$STASSID\\\" \
-DSTAPSK=\\\"$STAPSK\\\" \
-DCVERSION=$CVERSION \
-DSRVNAME=\\\"$SRVNAME\\\" \
-DSRVPORT=$SRVPORT \
"

platformio run

cp .pio/build/esp32cam/firmware.bin /output/
/root/create_manifest.py /output/manifest

