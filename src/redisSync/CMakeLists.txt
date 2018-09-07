
set_source_files_properties(redisClient.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(redisTest.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")

set(REDISYNC_LIB_SRCS
  redisClient.cpp
)

set(REDISYNC_TEST_SRCS
  redisTest.cpp
  redisClient.cpp
)

set(REDISYNC_TEST_LIBS
  hiredis
  dl
  rt
  z
)

add_library(redisSync ${REDISYNC_LIB_SRCS})

add_executable(redisSyncTest ${REDISYNC_TEST_SRCS})
target_link_libraries(redisSyncTest ${REDISYNC_TEST_LIBS})
