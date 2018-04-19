
set(CMAKE_RELEASE_BUILD "ON")

set_source_files_properties(src/ZooKafkaGet.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(src/ZooKfkTopicsGet.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion -Wno-deprecated-declarations")
set_source_files_properties(src/ZooKfkTopicsPop.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

set_source_files_properties(src/ZooKafkaPut.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(src/ZooKfkTopicsPush.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(src/ZooKfkProducer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(src/ZooKfkConsumer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

#set_source_files_properties(kfk_producer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
#set_source_files_properties(kfk_consumer.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")

set(ZOOKAFKA_SRCS  
  src/ZooKfkProducer.cpp
  src/ZooKfkTopicsPush.cpp
  src/ZooKafkaPut.cpp
  src/ZooKfkTopicsPop.cpp
)

if(CMAKE_RELEASE_BUILD)
	message(STATUS "Cmake zookeeper kafka lib release type")
else()
	message(STATUS "Cmake zookeeper kafka lib debug type")
	add_definitions(-DSHOW_DEBUG_MESSAGE)
	add_definitions(-DSHOW_ERROR_MESSAGE)
	
	add_executable(ZooKfkTest
	 test/ZooKfkMain.cpp
	 test/ConfigFile.cpp
	 src/ZooKfkProducer.cpp
	 src/ZooKfkTopicsPush.cpp
	 src/ZooKafkaPut.cpp
	 src/ZooKfkTopicsPop.cpp
	 )
	 
	target_link_libraries(ZooKfkTest
	 common
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
endif()

add_library(zookafka ${ZOOKAFKA_SRCS})

#add_executable(kfk_producer kfk_producer.cpp ZooKafkaPut.cpp ZooKfkTopicsPush.cpp)
#target_link_libraries(kfk_producer common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)

#add_executable(kfk_consumer kfk_consumer.cpp ZooKafkaGet.cpp ZooKfkTopicsGet.cpp ZooKfkTopicsPop.cpp ZooKfkConsumer.cpp)
#target_link_libraries(kfk_consumer common zookeeper_mt rdkafka++ rdkafka jansson crypto ssl sasl2 pthread dl rt z)

#add_executable(kfkzookeeper kfkzookeeper.cpp ZookeepKafka.cpp)
#target_link_libraries(kfkzookeeper common zookeeper_mt rdkafka jansson crypto ssl sasl2 pthread dl rt z)
