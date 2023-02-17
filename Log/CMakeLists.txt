set(Log
AsyncLogging.cpp
LogFile.cpp
Logger.cpp
LogStream.cpp
)
add_library(XWebServer_Log ${Log})

target_include_directories(
    XWebServer_Log
    PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(XWebServer_Log pthread)