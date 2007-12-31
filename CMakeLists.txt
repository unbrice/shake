CMAKE_MINIMUM_REQUIRED (VERSION 2.4)
set (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

project (Shake)
set (CPACK_VERSION_MAJOR "0")
set (CPACK_VERSION_MINOR "30")
set (CPACK_PACKAGE_CONTACT "Brice Arnould <brice.arnould+shake@gmail.com>")
set (CMAKE_C_FLAGS "$ENV{CFLAGS}")

set (VERSION "${CPACK_VERSION_MAJOR}.${CPACK_VERSION_MINOR}")


# Targets
add_executable (shake executive.c judge.c linux.c main.c msg.c signals.c)
add_executable (unattr executive.c linux.c signals.c unattr.c)
#MACRO (call_unix2man_after command)
#  get_target_property(exe ${command} LOCATION)
#  add_custom_command(TARGET ${command}
#    POST_BUILD
#    COMMAND help2man    ARGS -s8 -N -i ${CMAKE_CURRENT_SOURCE_DIR}/doc/fdl-man_insert -I ${CMAKE_CURRENT_SOURCE_DIR}/doc/${command}-man_insert -o ${CMAKE_CURRENT_BINARY_DIR}/${command}.8 ${exe}
#    COMMENT Build manpage for shake)
#ENDMACRO ()
MACRO (call_unix2man_after command)
  get_target_property(exe ${command} LOCATION)
  add_custom_command(TARGET ${command}
    POST_BUILD
    COMMAND help2man    ARGS -s8 -N -i ${CMAKE_CURRENT_SOURCE_DIR}/doc/fdl-man_insert -I ${CMAKE_CURRENT_SOURCE_DIR}/doc/${command}-man_insert -o ${CMAKE_CURRENT_BINARY_DIR}/${command}.8 ${exe}
    COMMENT Build manpage for shake)
ENDMACRO ()
call_unix2man_after(shake)
call_unix2man_after(unattr)

# Define how things are built
target_link_libraries(shake attr)
target_link_libraries(unattr attr)
IF (CMAKE_SYSTEM_NAME MATCHES Linux)
  add_definitions("-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64")  # For 64 bits files
  add_definitions("-D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L") # For posix_fallocate
ELSE ()
  message (STATUS "For now, shake has only been tested under GNU/Linux. I would be really interested by your experience under other OS, if you have time to write me at <brice.arnould+shake@gmail.com>")
ENDIF ()

IF (CMAKE_COMPILER_IS_GNUCC)
  list (APPEND CMAKE_C_FLAGS "-std=gnu99 -O2 -Wall -pedantic-errors -Wcast-align -Wpointer-arith -Wbad-function-cast")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -ggdb3 -Wconversion -Werror")
ENDIF ()

# http://www.cmake.org/Wiki/CMake_HowToDoPlatformChecks
INCLUDE (CheckSymbolExists)
check_symbol_exists (fallocate stdlib.h HAVE_FALLOCATE)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/build/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  ESCAPE_QUOTES)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

# *Define what to install and where
install (TARGETS shake unattr
  DESTINATION bin)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/shake.8 ${CMAKE_CURRENT_BINARY_DIR}/unattr.8
  DESTINATION share/man/man8)
# Debian
#set (DEBIAN_PACKAGE_SECTION "admin")
#set (DEBIAN_PACKAGE_BUILDS_DEPENDS "debhelper (>> 5.0.0) libattr1-dev (>= 2.0.0), cmake (>= 2.6)")
#set (DEBIAN_PACKAGE_DEPENDS "libattr1 (>= 2.0.0)")
# Packages
SET(CPACK_PACKAGE_VERSION_MAJOR ${CPACK_VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${CPACK_VERSION_MINOR})
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt)
#set (CPACK_GENERATOR "STGZ;TGZ;TZ")
INCLUDE(CPack)
