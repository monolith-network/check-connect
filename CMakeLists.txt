cmake_minimum_required(VERSION 3.8)

project(check-connect)

set(CMAKE_CXX_STANDARD 20)

#
# Setup build type 'Release vs Debug'
#
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

#
#  Find PkgConfig
#
find_package(PkgConfig REQUIRED)
pkg_check_modules(libcurl REQUIRED IMPORTED_TARGET libcurl>=7.17.0)

#
# Find Crate
#
find_package(crate REQUIRED)
include_directories(${CRATE_INCLUDE_DIRS})

#
# Find Threads
#
find_package (Threads REQUIRED)

#
# Find Nettle
#
find_package(Nettle REQUIRED)
include_directories(${NETTLE_INCLUDE_DIRS})

#
#  Ensure that tld includes available everywhere
#
include_directories(${CMAKE_SOURCE_DIR}/src)

#
#  Create the app
#
add_executable(check-connect
         src/main.cpp)

#
#  Link 
#
target_link_libraries(check-connect
        PRIVATE
        ${CRATE_LIBRARIES}
        ${NETTLE_LIBRARIES}
        Threads::Threads)

#
#  Copy configs
#  
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/check_connect-config.toml
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#
#  Copy rules
#  
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/check_connect-rules.lua
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
