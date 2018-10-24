#!/bin/bash

ZOOKEEPER="192.169.0.61:2181,192.169.0.62:2181,192.169.0.63:2181"
KAFKA_HOME="/usr/local/kafka"
###4427 3301 rptsyncout
mgateList="4427 3301";

${KAFKA_HOME}/bin/kafka-topics.sh --create --zookeeper $ZOOKEEPER --replication-factor 3 --partitions 3 --topic rptsyncout

for i in $mgateList
do
    ${KAFKA_HOME}/bin/kafka-topics.sh --create --zookeeper $ZOOKEEPER --replication-factor 3 --partitions 3 --topic mgate_$i
done
