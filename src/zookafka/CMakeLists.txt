
#set_source_files_properties(RedisClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")


set_source_files_properties(ZooKafkaPut.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(ZooKafkaGet.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(ZooKfkTopicsPop.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(ZooKfkTopicsPush.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(kfk_producer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set(ZOOKAFKA_SRCS
  ZooKafkaGet.cpp
  ZooKafkaPut.cpp
  ZookeepKafka.cpp
  ZooKfkTopicsPop.cpp
  ZooKfkTopicsPush.cpp
)

add_library(zookafka ${ZOOKAFKA_SRCS})

add_executable(kfk_producer kfk_producer.cpp ZooKafkaPut.cpp)
target_link_libraries(kfk_producer common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)

add_executable(kfk_consumer kfk_consumer.cpp ZooKfkTopicsPop.cpp)
target_link_libraries(kfk_consumer common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)

add_executable(kfkzookeeper kfkzookeeper.cpp ZookeepKafka.cpp)
target_link_libraries(kfkzookeeper common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)
