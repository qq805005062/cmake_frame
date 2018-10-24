
#set(CURLHTTPCLI_SOURCES_FILE
#  src/Daemon.cpp
#  src/Exception.cpp
#  src/Condition.cpp
#  src/Thread.cpp
#  src/ThreadPool.cpp
#  src/AsyncCurlHttp.cpp
#  src/CurlHttpCli.cpp
#)

set(CURLHTTPCLI_SOURCES_FILE
  src/CurlHttpCli.cpp
  src/AsyncCurlHttp.cpp
  src/ThreadPool.cpp
  src/Thread.cpp
  src/Condition.cpp
  src/Exception.cpp
  src/Daemon.cpp
)

set(CURLHTTPCLI_TEST_FILE
  src/CurlHttpCli.cpp
  src/AsyncCurlHttp.cpp
  src/ThreadPool.cpp
  src/Thread.cpp
  src/Condition.cpp
  src/Exception.cpp
  src/Daemon.cpp
  main_test.cpp
)

set(CURLHTTPCLI_TEST_LIB
  curlhttpcli
  curl
  crypto
  event_core
  event_pthreads
  pthread
  ssl
  z
  dl
  rt
)

add_library(curlhttpcli ${CURLHTTPCLI_SOURCES_FILE})

add_executable(curlHttpCliTest ${CURLHTTPCLI_TEST_FILE})
target_link_libraries(curlHttpCliTest ${CURLHTTPCLI_TEST_LIB})
install(TARGETS curlHttpCliTest DESTINATION bin)

#add_library(curlhttpcli ${CURLHTTPCLI_SOURCES_FILE})
