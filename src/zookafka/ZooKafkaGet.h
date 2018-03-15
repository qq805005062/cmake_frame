
#include <stdio.h>
#include <string>
#include <vector>

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

class ZooKafkaGet
{
public:

	ZooKafkaGet();

	~ZooKafkaGet();
	
	int init(const std::string& zookeepers,
			  const std::string& topic,
			  const std::string& groupId,
			  const std::vector<int>& partitions = { 0 },
	          int64_t startOffset = static_cast<int64_t>(RD_KAFKA_OFFSET_STORED),
	          size_t messageMaxSize = 4 * 1024 * 1024);

	int get(std::string& data, int64_t& offset,std::string* key = NULL);

	void destroy();

private:
	
	std::vector<int> partitions_;

	rd_kafka_t* handler_;
	rd_kafka_conf_t* conf_;
	rd_kafka_topic_conf_t* tconf_;
	rd_kafka_topic_partition_list_t* topics_;
};

}