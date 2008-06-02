# -*- cmake -*-
include(Linking)
# We don't prebuild our own MySQL client library.

set(MYSQL_FIND_QUIETLY ON)
set(MYSQL_FIND_REQUIRED ON)

if (WINDOWS)
  set(MYSQL_LIBRARIES mysqlclient)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
elseif (DARWIN)
  set(MYSQL_INCLUDE_DIR ${LIBS_PREBUILT_DIR}/${LL_ARCH_DIR}/include)
  set(MYSQL_LIBRARIES
    optimized ${LIBS_PREBUILT_DIRS_RELEASE}/mysqlclient
    debug ${LIBS_PREBUILT_DIRS_DEBUG}/mysqlclient
    )
else (WINDOWS)
    set(MYSQL_FIND_REQUIRED)
    include(FindMySQL)
endif (WINDOWS)