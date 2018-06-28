
set(CMAKE_RELEASE_BUILD "ON")

if(CMAKE_RELEASE_BUILD)
	message(STATUS "Cmake zookeeper kafka lib release type")
else()
	message(STATUS "Cmake zookeeper kafka lib debug type")
	add_definitions(-DSHOW_DEBUG_MESSAGE)
	add_definitions(-DSHOW_ERROR_MESSAGE)
endif()

set(ZOOKAFKA_LIB_SRCS
  src/ZooKfkTopicsPush.cpp
  src/ZooKfkTopicsPop.cpp
)

set(ZOOKAFKA_TEST_SRCS
  test/ZooKfkMain.cpp
  test/ConfigFile.cpp
  test/Condition.cpp
  test/Exception.cpp
  test/Thread.cpp
  test/ThreadPool.cpp
  test/Timestamp.cpp
  src/ZooKfkTopicsPush.cpp
  src/ZooKfkTopicsPop.cpp
)

set(ZOOKAFKA_TEST_LIBS
  zookeeper_mt
  rdkafka++
  rdkafka
  jansson
  crypto
  ssl
  sasl2
  pthread
  dl
  rt
  z
)

add_library(zookafka ${ZOOKAFKA_LIB_SRCS})

add_executable(ZooKfkTest ${ZOOKAFKA_TEST_SRCS})

target_link_libraries(ZooKfkTest ${ZOOKAFKA_TEST_LIBS})


#set_source_files_properties(src/ZooKafkaGet.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
#set_source_files_properties(src/ZooKfkTopicsGet.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion -Wno-deprecated-declarations")
#set_source_files_properties(src/ZooKfkTopicsPop.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

#set_source_files_properties(src/ZooKafkaPut.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
#set_source_files_properties(src/ZooKfkTopicsPush.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
#set_source_files_properties(src/ZooKfkProducer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
#set_source_files_properties(src/ZooKfkConsumer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

#set_source_files_properties(kfk_producer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
#set_source_files_properties(kfk_consumer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")

#add_executable(kfk_producer kfk_producer.cpp ZooKafkaPut.cpp ZooKfkTopicsPush.cpp)
#target_link_libraries(kfk_producer common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)

#add_executable(kfk_consumer kfk_consumer.cpp ZooKafkaGet.cpp ZooKfkTopicsGet.cpp ZooKfkTopicsPop.cpp ZooKfkConsumer.cpp)
#target_link_libraries(kfk_consumer common zookeeper_mt rdkafka++ rdkafka jansson crypto ssl sasl2 pthread dl rt z)

#add_executable(kfkzookeeper kfkzookeeper.cpp ZookeepKafka.cpp)
#target_link_libraries(kfkzookeeper common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)
