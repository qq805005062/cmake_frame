
set(CURLHTTPCLI_SOURCES_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/AsyncCurlHttp.cpp
  src/CurlHttpCli.cpp
)

set(CURLHTTPCLI_TEST_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/AsyncCurlHttp.cpp
  src/CurlHttpCli.cpp
  main_test.cpp
)

set(CURLHTTPCLI_TEST_LIB
  curl
  cares
  event_core
  event_pthreads
  pthread
  ssl
  crypto
  z
  dl
  rt
)

add_library(curlhttpcli ${CURLHTTPCLI_SOURCES_FILE})

add_executable(curlHttpCliTest ${CURLHTTPCLI_TEST_FILE})
target_link_libraries(curlHttpCliTest ${CURLHTTPCLI_TEST_LIB})
install(TARGETS curlHttpCliTest DESTINATION bin)

add_executable(curlSyncTest CurlSyncTesr.cpp)
target_link_libraries(curlSyncTest ${CURLHTTPCLI_TEST_LIB})
install(TARGETS curlSyncTest DESTINATION bin)
