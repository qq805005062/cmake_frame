
#include <stdio.h>
#include <string>

#include "librdkafka/rdkafka.h"

#include "zookeeper/zookeeper.h"
#include "zookeeper/zookeeper.jute.h"

#define SHOW_DEBUG		1

#ifdef SHOW_DEBUG
#define PDEBUG(fmt, args...)	fprintf(stderr, "%s :: %s() %d: DEBUG " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
#else
#define PDEBUG(fmt, args...)
#endif

#define PERROR(fmt, args...)	fprintf(stderr, "%s :: %s() %d: ERROR " fmt,__FILE__, \
									__FUNCTION__, __LINE__, ## args)
									
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