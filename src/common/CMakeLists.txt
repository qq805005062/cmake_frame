
set_source_files_properties(MysqlEncryPass.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(process_stat_linux_ex.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion -Wno-fpermissive")

set(common_SRCS
  Daemon.cpp
  Error.cpp
  DirUtil.cpp
  StringUtility.cpp
  TimeUtility.cpp
#  Log4cxxImpl.cpp
  Condition.cpp
  Thread.cpp
  ThreadPool.cpp
  Exception.cpp
  CountDownLatch.cpp
  Timestamp.cpp
  Base64.cpp
  MysqlEncryPass.cpp
#  Des.cpp
  AES.cpp
  process_stat_linux_ex.cpp
#  Monitor.cpp
)

add_library(common ${common_SRCS})

#install(TARGETS common DESTINATION lib)
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/common)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()

