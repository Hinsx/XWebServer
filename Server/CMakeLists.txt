set(Base
Threadpool.cpp
Acceptor.cpp
HttpResponse.cpp
CurrentThread.cpp
EventLoop.cpp
Timer.cpp
HttpContext.cpp
Thread.cpp
TimerQueue.cpp
Buffer.cpp
SQLConnection.cpp
Poller.cpp
HttpServer.cpp
HttpConnection.cpp
Timestamp.cpp
Channel.cpp
SQLpool.cpp
InetAddress.cpp
Socket.cpp
Config.cpp
)
add_library(XWebServer_Base ${Base})

target_include_directories(
    XWebServer_Base
    PUBLIC
    ${PROJECT_SOURCE_DIR}
)
target_link_libraries(XWebServer_Base XWebServer_Log pthread mysqlclient)