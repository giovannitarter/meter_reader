#!/bin/bash


WORKDIR="$(cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd)"

pushd "$WORKDIR"

pushd docker
docker build -t espcam_builder .
popd

if [ ! -e "VERSION" ];
then
    echo "0" > "VERSION"
fi
CVERSION="$(cat VERSION)"
CVERSION="$((CVERSION + 1))"

rm -rf "$WORKDIR/output"
mkdir -p "$WORKDIR/output"
docker run \
    --rm \
    -it \
    --env-file "$WORKDIR/.env" \
    -e CVERSION="$CVERSION" \
    -v"$WORKDIR/output":/output \
    -v"$WORKDIR/project":/root/project \
    espcam_builder


if [ -e "$WORKDIR/output/firmware.bin" ];
then

    echo "Version $CVERSION build successfully"

    echo "$CVERSION" > VERSION
    mkdir -p "$WORKDIR/../mr_server/firmware/"
    cp "$WORKDIR/output/manifest" "$WORKDIR/../mr_server/firmware/"
    cp "$WORKDIR/output/firmware.bin" "$WORKDIR/../mr_server/firmware/"
    
    echo ""
    echo "Manifest:"
    cat "$WORKDIR/output/manifest"
    echo ""
    
    rm -rf "$WORKDIR/output"

else
    echo "Error building firmware"
    echo "Not deploying"
fi
