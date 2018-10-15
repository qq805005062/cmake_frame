
set(CURLHTTPCLI_SOURCES_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/AsyncCurlHttp.cpp
  src/CurlHttpCli.cpp
)

add_library(curlhttpcli ${CURLHTTPCLI_SOURCES_FILE})
