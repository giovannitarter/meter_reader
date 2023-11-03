#!/bin/bash


if [ ! -e "environment" ];
then
    cp "environment.sample" "environment"
fi

source environment

if [ -e "VERSION" ];
then
    VERSION="$(cat VERSION 2>/dev/null)"
else
    VERSION=0
    echo "$VERSION" > VERSION
fi

echo "-DSTASSID=\\\"$STASSID\\\" -DSTAPSK=\\\"$STAPSK\\\" -DSRVNAME=\\\"$SRVNAME\\\" -DSRVPORT=$SRVPORT -DSLEEP_TIME=$SLEEP_TIME -DCVERSION=$VERSION"

