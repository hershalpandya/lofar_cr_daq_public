cmake_minimum_required(VERSION 2.8.9)
project(lora_daq)
include_directories(include/)

set (CMAKE_CXX_STANDARD 11)
if (APPLE)
  set (CMAKE_CXX_COMPILER /usr/bin/g++)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libc++")
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")
endif()


#for coma use: set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -g")
#set( CMAKE_VERBOSE_MAKEFILE on )
# -g for debugging later using gdb. remove afterwards.

if(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  set(CMAKE_MACOSX_RPATH 1)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}/usr/local/share/root/cmake/)
else()
  # for lora pc old
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}/home/hpandya/root/root/etc/cmake/)
  # for lora pc new rack server
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} /home/software/root/build/)
endif()

#list(APPEND CMAKE_PREFIX_PATH /vol/optcoma/offline/ape/External/root/5.34.18/bin/)
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}$ENV{ROOTSYS}/etc/cmake/)

find_package(ROOT REQUIRED)

if(APPLE)
  # for MacOS X or iOS, watchOS, tvOS (since 3.10.3)
  include_directories(/usr/local/Cellar/root/6.18.00/include/root)
else()
  # for lora pc
  include_directories(${ROOT_INCLUDE_DIR})
  # for lora pc new rack server
  include_directories(/home/software/root/build/include)
endif()


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}/home/hpandya/boost/boost_1_68_0/) # for lora main pc
find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

# added this next line for lora main pc , new rack server
include_directories(/home/software/boost_install/include/)

add_library(functionslib src/functions_library.cxx)
add_library(buffer src/Buffer.cxx)
add_library(lorav1 SHARED src/LORA_STATION_V1.cxx src/LORA_STATION.cxx)
add_library(lorav2 SHARED src/LORA_STATION_V2.cxx src/LORA_STATION.cxx)
add_library(operations src/OPERATIONS.cxx )
add_library(socket_calls src/Socket_Calls.cxx)
add_library(structs src/Structs.cxx)
add_library(lofar src/LOFAR_Communications.cxx)
add_executable(main src/main.cxx)
# add_executable(main_muon_calib src/main_muon_calib.cxx)
add_executable(main_elec_calib src/main_elec_calib.cxx)

if (APPLE)
  target_link_libraries(lorav1 functionslib)
  target_link_libraries(lorav2 functionslib)
  target_link_libraries(socket_calls functionslib)
  target_link_libraries(lorav1 socket_calls)
  target_link_libraries(lorav2 socket_calls)
  target_link_libraries(lorav1 buffer)
  target_link_libraries(lorav2 buffer)
  target_link_libraries(buffer functionslib)
endif()

target_link_libraries(main operations lorav1 lorav2 socket_calls buffer structs functionslib lofar ${ROOT_LIBRARIES} ${Boost_LIBRARIES})
# target_link_libraries(main_muon_calib operations lorav1 lorav2 socket_calls buffer structs functionslib lofar ${ROOT_LIBRARIES} ${Boost_LIBRARIES})
target_link_libraries(main_elec_calib operations lorav1 lorav2 socket_calls buffer structs functionslib lofar ${ROOT_LIBRARIES} ${Boost_LIBRARIES})
