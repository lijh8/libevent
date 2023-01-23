# apt install libevent-dev

#cmake_minimum_required(VERSION 3.24)
#project(myproject VERSION 1.0)
#set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
#set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
#set(CMAKE_CXX_STANDARD 17) # .cpp
#set(CMAKE_CXX_STANDARD_REQUIRED TRUE) # .cpp
#file(GLOB SOURCES *.cpp) # .cpp

find_package(PkgConfig)
pkg_search_module(LIBEVENT libevent)
pkg_search_module(LIBEVENT_CORE libevent_core)

#add_executable(main ${SOURCES})
add_executable(server server.cpp)
add_executable(client client.cpp)
#add_library(main SHARED ${SOURCES})
#set_target_properties(main PROPERTIES VERSION 1.2.3 SOVERSION 1.2)

target_link_libraries(server PUBLIC ${LIBEVENT_LIBRARIES} ${LIBEVENT_CORE_LIBRARIES})
target_link_directories(server PUBLIC ${LIBEVENT_LIBRARY_DIRS} ${LIBEVENT_CORE_LIBRARY_DIRS})
target_include_directories(server PUBLIC ${LIBEVENT_INCLUDE_DIRS} ${LIBEVENT_CORE_INCLUDE_DIRS})

target_link_libraries(client PUBLIC ${LIBEVENT_LIBRARIES} ${LIBEVENT_CORE_LIBRARIES})
target_link_directories(client PUBLIC ${LIBEVENT_LIBRARY_DIRS} ${LIBEVENT_CORE_LIBRARY_DIRS})
target_include_directories(client PUBLIC ${LIBEVENT_INCLUDE_DIRS} ${LIBEVENT_CORE_INCLUDE_DIRS})

#add_subdirectory(./foo)

# make DESTDIR=~/myproject install
install(TARGETS server client # foo
    )
