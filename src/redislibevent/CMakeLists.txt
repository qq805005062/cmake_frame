
set_source_files_properties(RedisAsync.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-unused-function -Wno-conversion")
set_source_files_properties(redislibevent.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-unused-function -Wno-conversion")

set(REDISASYNC_SRCS
  RedisAsync.cpp
)

add_library(redisasync ${REDISASYNC_SRCS})

add_executable(redislibevent redislibevent.cpp RedisAsync.cpp)
target_link_libraries(redislibevent common hiredis event_core event_pthreads pthread dl rt z)


