#!/bin/bash

set -x

rootDir=`pwd`
dataDir=$rootDir/logs

find $dataDir -name "controller.log.*" -mtime +7 | xargs rm -f
find $dataDir -name "server.log.*" -mtime +7 | xargs rm -f
find $dataDir -name "state-change.log.*" -mtime +7 | xargs rm -f
