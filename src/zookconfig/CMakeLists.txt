
#set(CMAKE_RELEASE_BUILD "ON")

if(CMAKE_RELEASE_BUILD)
	message(STATUS "Cmake zookeeper config lib release type")
else()
	message(STATUS "Cmake zookeeper config lib debug type")
	add_definitions(-DSHOW_PRINTF_MESSAGE)
endif()

set(ZOOCONFIG_LIB_SRCS
  ZookConfig.cpp
)

set(ZOOCONFIG_TEST_SRCS
  test/ZookConMain.cpp
  ZookConfig.cpp
)

set(ZOOCONFIG_TEST_LIBS
  zookeeper_mt
  crypto
  ssl
  sasl2
  pthread
  dl
  rt
  z
)

add_library(zooconfig ${ZOOCONFIG_LIB_SRCS})

add_executable(ZooConfTest ${ZOOCONFIG_TEST_SRCS})
target_link_libraries(ZooConfTest ${ZOOCONFIG_TEST_LIBS})
install(TARGETS ZooConfTest DESTINATION bin)

