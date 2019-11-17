
set(REMAIN_MGR_FILE
  src/Base64.cpp
  src/WriteFileMgr.cpp
  src/ReadFileMgr.cpp
  src/ReadDirMgr.cpp
  src/RemainMgr.cpp
  src/RemainObjMgr.cpp
)

set(REMAIN_MGR_TEST_FILE
  src/WriteFileMgr.cpp
  src/ReadFileMgr.cpp
  src/ReadDirMgr.cpp
  src/RemainMgr.cpp
  src/RemainObjMgr.cpp
)


set(REMAIN_MGR_TEST_LIB
  pthread
  z
  dl
  rt
)

add_library(remainmgr ${REMAIN_MGR_FILE})

#add_executable(remainTest ${REMAIN_MGR_TEST_FILE})
#target_link_libraries(remainTest ${REMAIN_MGR_TEST_LIB})
#install(TARGETS remainTest DESTINATION bin)
