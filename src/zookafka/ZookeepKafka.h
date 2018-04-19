
#include <stdio.h>
#include <string>

#include "librdkafka/rdkafka.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"


namespace ZOOKEEPERKAFKA
{

class ZookeepKafka
{
public:
	ZookeepKafka();
	~ZookeepKafka();
	
private:
	rd_kafka_t* handler_;
	rd_kafka_conf_t* conf_;
	rd_kafka_topic_conf_t* tconf_;
	rd_kafka_topic_partition_list_t* topics_;
};

}