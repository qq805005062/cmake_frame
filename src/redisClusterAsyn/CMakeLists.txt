
set(LIBEVENT_REDIS_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/RedisTools.cpp
  src/RedisClient.cpp
  src/LibeventIo.cpp
  src/ClusterRedisAsync.cpp
)

set(LIBEVENT_REDIS_TEST_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/RedisTools.cpp
  src/RedisClient.cpp
  src/LibeventIo.cpp
  src/ClusterRedisAsync.cpp
  TestMain.cpp
)

set(LIBEVENT_REDIS_LIB_FILE
  hiredis
  event_core
  pthread
  z
  dl
  rt
)

add_library(redisAsyn ${LIBEVENT_REDIS_FILE})

add_executable(redisAsynTest ${LIBEVENT_REDIS_TEST_FILE})
target_link_libraries(redisAsynTest ${LIBEVENT_REDIS_LIB_FILE})
install(TARGETS redisAsynTest DESTINATION bin)
