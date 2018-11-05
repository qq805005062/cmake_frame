
set(LIBEVENT_TCPCLI_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/TcpClient.cpp
  src/LibeventIo.cpp
  src/LibeventTcpCli.cpp
)

set(LIBEVENT_TCP_TEST_FILE
  src/Exception.cpp
  src/Condition.cpp
  src/Thread.cpp
  src/ThreadPool.cpp
  src/TcpClient.cpp
  src/LibeventIo.cpp
  src/LibeventTcpCli.cpp
  main_test.cpp
)

set(LIBEVENT_TCP_LIB_FILE
  event_core
  pthread
  z
  dl
  rt
)

add_library(eventcpcli ${LIBEVENT_TCPCLI_FILE})

add_executable(tcpCliTest ${LIBEVENT_TCP_TEST_FILE})
target_link_libraries(tcpCliTest ${LIBEVENT_TCP_LIB_FILE})
install(TARGETS tcpCliTest DESTINATION bin)
