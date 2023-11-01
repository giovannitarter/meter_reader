#!/bin/bash


source ../environment

if [ -e "../VERSION" ];
then
    VERSION="$(cat ../VERSION 2>/dev/null)"
else
    VERSION=0
fi

echo "-DSTASSID=\\\"$STASSID\\\" -DSTAPSK=\\\"$STAPSK\\\" -DSRVNAME=\\\"$SRVNAME\\\" -DSRVPORT=$SRVPORT -DSLEEP_TIME=$SLEEP_TIME -DCVERSION=$VERSION"

VERSION=$((VERSION + 1))
echo "$VERSION" > ../VERSION


