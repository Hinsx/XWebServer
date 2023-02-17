cmake_minimum_required(VERSION 3.16.3)

project(XWebServer)

option(BLANK_RESPONSE "Do not response with any file" OFF)
if(BLANK_RESPONSE)
    message("Select option BLANK_RESPONSE")
    add_definitions(-DBLANK_RESPONSE)
endif()
option(TRACE "Trace the excution process using cout" OFF)
if(TRACE)
    message("Select option Trace")
    add_definitions(-DMYTRACE)
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  #set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 " CACHE STRING "Set C++ Compiler Flags" FORCE)




add_subdirectory(Log)
add_subdirectory(Server)

add_executable(${PROJECT_NAME} main.cpp)


target_link_libraries(
    ${PROJECT_NAME}
    ${PROJECT_NAME}_Base
    ${PROJECT_NAME}_Log
)
message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("BLANK_RESPONSE: ${BLANK_RESPONSE}")
message("TRACE: ${TRACE}")