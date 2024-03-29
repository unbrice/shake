#  Copyright (C) 2006-2022 Brice Arnould.
# 
#  This file is part of ShaKe.
#
#  ShaKe is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#  You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

#### MACROS AND WORKAROUNDS ####
CMAKE_MINIMUM_REQUIRED (VERSION 3.14) # Released on 2020-10-02
set (CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

MACRO (add_help2man_manpage file command)
  add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${file}
    COMMAND ${HELP2MAN_LOCATION} ARGS -s8 -N -i ${CMAKE_CURRENT_SOURCE_DIR}/doc/fdl-man_insert -I ${CMAKE_CURRENT_SOURCE_DIR}/doc/${command}-man_insert -o ${CMAKE_CURRENT_BINARY_DIR}/${file} $<TARGET_FILE:${command}>
    DEPENDS ${command}
    COMMENT "Building manpage for ${command}")
ENDMACRO ()

#### PROJECT CONFIG ####
project (Shake C)
set (CPACK_VERSION_MAJOR "1")
set (CPACK_VERSION_MINOR "0")
set (CPACK_PACKAGE_CONTACT "Brice Arnould <unbrice@vleu.net>")

set (VERSION "${CPACK_VERSION_MAJOR}.${CPACK_VERSION_MINOR}")

#### System checks ####
find_program (HELP2MAN_LOCATION help2man)
IF (NOT HELP2MAN_LOCATION)
  message (SEND_ERROR "Cannot find help2man. Please install it.")
ENDIF ()

#### Targets ####
add_executable (shake executive.c judge.c linux.c main.c msg.c signals.c)
add_executable (unattr executive.c linux.c signals.c unattr.c)
add_help2man_manpage (shake.8 shake)
add_help2man_manpage (unattr.8 unattr)
add_custom_target (doc ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shake.8
          ${CMAKE_CURRENT_BINARY_DIR}/unattr.8)

#### Platform Specific ####

## LINUX  ##
IF (CMAKE_SYSTEM_NAME MATCHES Linux)
  add_definitions (-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64)  # For 64 bits files
  add_definitions (-D_XOPEN_SOURCE=600 -D_POSIX_C_SOURCE=200112L) # For posix_fallocate
  list (APPEND CMAKE_REQUIRED_LIBRARIES attr)
  target_link_libraries (shake attr)
  target_link_libraries (unattr attr)
ELSE ()
  message ("For now, shake has only been tested under GNU/Linux.")
ENDIF ()

## GCC  ##
IF (CMAKE_COMPILER_IS_GNUCC)
  set (CMAKE_C_FLAGS "$ENV{CFLAGS} -std=gnu99 -O2 -Wall -Wextra -Wcast-align -Wpointer-arith -Wbad-function-cast -Wno-sign-compare")
  set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -O0 -ggdb3 -pedantic -Wconversion -Werror -DDEBUG=1")
ENDIF ()

## ALL  ##
INCLUDE (CheckIncludeFiles)
INCLUDE (CheckFunctionExists)
check_function_exists (attr_setf HAVE_LIBATTR)
check_function_exists (fallocate HAVE_FALLOCATE)
configure_file (${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  ESCAPE_QUOTES)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

## SANITY CHECK ##
IF (NOT HAVE_LIBATTR)
  message (SEND_ERROR "You need libattr headers for Shake to build. They might be in a package named libattr1-dev or libattr-devel.")
ENDIF()

#### Installation ####
install (TARGETS shake unattr
  DESTINATION bin)
install (FILES ${CMAKE_CURRENT_BINARY_DIR}/shake.8 ${CMAKE_CURRENT_BINARY_DIR}/unattr.8
  DESTINATION share/man/man8)
## Packages ##
set (CPACK_PACKAGE_VERSION_MAJOR ${CPACK_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR ${CPACK_VERSION_MINOR})
set (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt)
set (CPACK_GENERATOR "STGZ;TGZ;TZ")
INCLUDE(CPack)
