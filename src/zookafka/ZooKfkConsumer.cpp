
#include "ZOOKEEPERKAFKA.h"

namespace KFK
{

static void err_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque)
{
	PERROR("%% ERROR CALLBACK: %s: %s: %s\n",rd_kafka_name(rk), rd_kafka_err2str(err), reason);
}

static int stats_cb(rd_kafka_t *rk, char *json, size_t json_len, void *opaque)
{
	/* Extract values for our own stats */
	if (json)
		PDEBUG("stats_cb: %s", json);
	return 0;
}

static void throttle_cb(rd_kafka_t *rk,
	const char *broker_name,
	int32_t broker_id,
	int throttle_time_ms,
	void *opaque)
{
	PDEBUG("%% THROTTLED %dms by %s (%\"PRId32\")\n", throttle_time_ms, broker_name, broker_id);
}

static void offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
	rd_kafka_topic_partition_list_t *offsets,
	void *opaque)
{
	int i;

	if (err || verbosity >= 2)
		PDEBUG("%% Offset commit of %d partition(s): %s\n",
			offsets->cnt, rd_kafka_err2str(err));

	for (i = 0; i < offsets->cnt; i++)
	{
		rd_kafka_topic_partition_t *rktpar = &offsets->elems[i];
		if (rktpar->err || verbosity >= 2)
			PDEBUG("%%  %s [%\"PRId32\"] @ %\"PRId64\": %s\n",
				rktpar->topic, rktpar->partition,
				rktpar->offset, rd_kafka_err2str(err));
	}
}


static void rebalance_cb(rd_kafka_t *rk,
	rd_kafka_resp_err_t err,
	rd_kafka_topic_partition_list_t *partitions,
	void *opaque) 
{
	switch (err)
	{
		case RD_KAFKA_RESP_ERR__ASSIGN_PARTITIONS:
			PERROR("%% Group rebalanced: %d partition(s) assigned\n",partitions->cnt);
			eof_cnt = 0;
			partition_cnt = partitions->cnt;
			rd_kafka_assign(rk, partitions);
			break;
		case RD_KAFKA_RESP_ERR__REVOKE_PARTITIONS:
			PERROR("%% Group rebalanced: %d partition(s) revoked\n",partitions->cnt);
			eof_cnt = 0;
			partition_cnt = 0;
			rd_kafka_assign(rk, NULL);
			break;
		default:
			break;
	}
}

void msg_consume_proc(rd_kafka_message_t* message, void* opaque)
{
	if (message && !message->err)
	{
		std::cout << "msg consume: " << static_cast<char*>(message->payload)
			<< " partition:" << message->partition
			<< " err:" << rd_kafka_err2str(message->err) << std::endl;

	}
	return;
}

int consumer::init(const std::string brokers, const std::string topic)
{
	if (NULL == m_conf)
		m_conf = rd_kafka_conf_new();

	rd_kafka_conf_set_error_cb(m_conf, err_cb);
	rd_kafka_conf_set_throttle_cb(m_conf, throttle_cb);
	rd_kafka_conf_set_offset_commit_cb(m_conf, offset_commit_cb);

	rd_kafka_conf_set(m_conf, "queued.min.messages", "1000000", NULL, 0);
	rd_kafka_conf_set(m_conf, "session.timeout.ms", "6000", NULL, 0);

	// [topic]
	if (NULL == m_topic_conf)
		m_topic_conf = rd_kafka_topic_conf_new();
	if (NULL == m_topic_conf)
		return -1;
	rd_kafka_topic_conf_set(m_topic_conf, "auto.offset.reset", "earliest",
		NULL, 0);

	// topics
	
	if (NULL == m_topics)
		m_topics = rd_kafka_topic_partition_list_new(1);
	if (NULL == m_topics)
		return -2;

	// topic
	rd_kafka_topic_partition_list_add(m_topics, topic.c_str(), static_cast<int32_t>(-1));//RD_KAFKA_PARTITION_UA);

	m_brokers = brokers;

	return 0;
}

	int consumer::start(std::string group, int64_t start_offset, consumer_interface* ex_consume_cb)
	{
		rd_kafka_resp_err_t err;
		char errstr[512];

		// group
		if (rd_kafka_conf_set(m_conf, "group.id", group.c_str(),
			errstr, sizeof(errstr)) !=
			RD_KAFKA_CONF_OK)
		{
			fprintf(stderr, "%% %s\n", errstr);
			return -3;
		}

		// …Ë÷√ªÿµ˜
		rd_kafka_conf_set_stats_cb(m_conf, stats_cb);
		rd_kafka_conf_set_rebalance_cb(m_conf, rebalance_cb);
		rd_kafka_conf_set_default_topic_conf(m_conf, m_topic_conf);

		
		/* Create Kafka handle */
		if (!(m_rk = rd_kafka_new(RD_KAFKA_CONSUMER, m_conf,
			errstr, sizeof(errstr))))
		{
			fprintf(stderr,
				"%% Failed to create Kafka consumer: %s\n",
				errstr);
			return -4;
		}

		
		/* Forward all events to consumer queue */
		rd_kafka_poll_set_consumer(m_rk);

		/* Add broker(s) */
		if (!m_brokers.empty() && rd_kafka_brokers_add(m_rk, m_brokers.c_str()) < 1)
		{
			fprintf(stderr, "%% No valid brokers specified\n");
			return -5;
		}

		err = rd_kafka_subscribe(m_rk, m_topics);
		if (err)
		{
			fprintf(stderr, "%% Subscribe failed: %s\n",
				rd_kafka_err2str(err));
			return -6;
		}
		fprintf(stderr, "%% Waiting for group rebalance..\n");

		

		if (!m_thread_spr)
		{
			m_bRun = true;
			m_thread_spr = std::make_shared<std::thread>([=]()
			{
				// m_cb_consumer.set_callback(ex_consume_cb);
				while (m_bRun)
				{
					/* Consume messages.
					* A message may either be a real message, or
					* an event (if rkmessage->err is set).
					*/
					rd_kafka_message_t *rkmessage;
					rkmessage = rd_kafka_consumer_poll(m_rk, 1000);
					if (rkmessage)
					{
						if (ex_consume_cb)
						{
							std::string errmsg = rd_kafka_err2str(rkmessage->err);
							std::string strmsg(static_cast<char*>(rkmessage->payload), rkmessage->len);
							ex_consume_cb->on_consumer(static_cast<int>(rkmessage->err), errmsg, strmsg, rkmessage->offset);
						}
						// m_cb_consumer.consume_cb(rkmessage, NULL);
						// msg_consume_proc(rkmessage, NULL);
						rd_kafka_message_destroy(rkmessage);
					}
				}

				std::cout << "consumer done!" << std::endl;
			});
		}
		
		return 0;
	}

int consumer::stop()
{
	m_bRun = false;

	m_thread_spr->join();
	if (m_rk)
	{
		rd_kafka_resp_err_t err = rd_kafka_consumer_close(m_rk);
		if (err)
		{
			fprintf(stderr, "%% Failed to close consumer: %s\n",
				rd_kafka_err2str(err));
		}
		rd_kafka_destroy(m_rk);

		m_rk = NULL;
	}

	if (m_topics)
	{
		rd_kafka_topic_partition_list_destroy(m_topics);
		m_topics = NULL;
	}
	/* Let background threads clean up and terminate cleanly. */
	
	rd_kafka_wait_destroyed(2000);

	m_thread_spr.reset();
	
	return 0;
}

	

int consumer::set(const std::string attr, const std::string sValue, std::string& errstr)
{
	if (!m_conf)
		return enum_err_set_INVALID;
	rd_kafka_conf_res_t ret = rd_kafka_conf_set(m_conf, attr.c_str(), sValue.c_str(), NULL, 0);
	return static_cast<int>(ret);
}

}

