
set_source_files_properties(AES.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(gzip.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(MD5.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set_source_files_properties(MysqlEncryPass.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion")
set_source_files_properties(process_stat_linux_ex.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-conversion -Wno-fpermissive")

set(common_SRCS
  Daemon.cpp
  Error.cpp
  DirUtil.cpp
  StringUtility.cpp
  TimeUtility.cpp
  Condition.cpp
  Thread.cpp
  ThreadPool.cpp
  Exception.cpp
  CountDownLatch.cpp
  Timestamp.cpp
  Base64.cpp
  MysqlEncryPass.cpp
  AES.cpp
  MD5.cpp
  gzip.cpp
  zip.cpp
  process_stat_linux_ex.cpp
)

add_library(common ${common_SRCS})

add_executable(modTest moduleTest.cpp)
target_link_libraries(modTest common crypto z dl rt)
install(TARGETS modTest DESTINATION bin)

#install(TARGETS common DESTINATION lib)
#file(GLOB HEADERS "*.h")
#install(FILES ${HEADERS} DESTINATION include/common)

if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()

