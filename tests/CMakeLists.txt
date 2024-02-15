cmake_minimum_required(VERSION 3.10)

# set the project name
project(rmlib)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# include folders
FILE(GLOB_RECURSE MY_HEADERS "../include/rmlib/*.h*")

# add the executable
if(MSVC)
   include_directories(../include ../wepoll ../catch2 ${OPENSSL_INCLUDE_DIR})
   add_executable(rmlib main.cpp ../wepoll/wepoll.c status-ut.cpp fstream-ut.cpp time-ut.cpp socket-ut.cpp ${MY_HEADERS} )
else()
   include_directories(../include ../catch2 ${OPENSSL_INCLUDE_DIR})
   add_executable(rmlib main.cpp status-ut.cpp fstream-ut.cpp time-ut.cpp socket-ut.cpp ${MY_HEADERS} )
endif()

target_link_libraries(rmlib ${OPENSSL_LIBRARIES})
