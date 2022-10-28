#!/bin/bash


pushd docker
docker build -t espcam_builder .
popd

WORKDIR="$(pwd)"


CVERSION="$(cat VERSION)"
CVERSION="$((CVERSION + 1))"

source $WORKDIR/.vars

rm -rf "$WORKDIR/output"
mkdir -p "$WORKDIR/output"
docker run -e SRVPORT="$SRVPORT" -e SRVNAME="$SRVNAME" -e CVERSION="$CVERSION" -e STASSID="$STASSID" -e STAPSK="$STAPSK" --rm -it -v"$WORKDIR/output":/output -v"$WORKDIR/build":/root/build espcam_builder


if [ -e "$WORKDIR/output/firmware.bin" ];
then

    echo "Version $CVERSION build successfully"

    echo "$CVERSION" > VERSION
    mkdir -p "$WORKDIR/../mr_server/firmware/"
    cp "$WORKDIR/output/manifest" "$WORKDIR/../mr_server/firmware/"
    cp "$WORKDIR/output/firmware.bin" "$WORKDIR/../mr_server/firmware/"
    rm -rf "$WORKDIR/output"

else
    echo "Error building firmware"
    echo "Not deploying"
fi
