#!/bin/bash

#zookeeperManage.sh
#

#snapshot file dir
dataDir=/usr/local/zookeeper/data/version-2
#tran log dir
dataLogDir=/usr/local/zookeeper/logs/version-2
#zk log dir
#logDir==/usr/local/zookeeper/logs
#Leave 66 files
count=66
count=$[$count+1]

ls -t $dataLogDir/snapshot.* | tail -n +$count | xargs rm -f
ls -t $dataDir/log.* | tail -n +$count | xargs rm -f
#ls -t $logDir/zookeeper.log.* | tail -n +$count | xargs rm -f 

#find /usr/local/zookeeper/data/version-2 -name "snap*" -mtime +1 | xargs rm -f
#find /usr/local/zookeeper/logs/version-2 -name "log*" -mtime +1 | xargs rm -f
#find /usr/local/zookeeper/logs -name "zookeeper.log.*" -mtime +1 | xargs rm –f
