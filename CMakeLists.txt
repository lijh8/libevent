#sudo apt install libevent-dev

#cmake_minimum_required(VERSION 3.24)
#project(myproject VERSION 1.0)
#set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
#set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -fsanitize=address)
endif()

#set(CMAKE_CXX_STANDARD 17) # .cpp
#set(CMAKE_CXX_STANDARD_REQUIRED TRUE) # .cpp
#file(GLOB SOURCES *.cpp) # .cpp

#add_executable(main ${SOURCES})
add_executable(server server.cpp)
add_executable(client client.cpp)
#add_library(main SHARED ${SOURCES})
#set_target_properties(main PROPERTIES VERSION 1.2.3 SOVERSION 1.2)

target_link_libraries(server PUBLIC      event_core)
target_link_directories(server PUBLIC    /home/ljh/Downloads/libevent/build/lib)
target_include_directories(server PUBLIC /home/ljh/Downloads/libevent/include
                                         /home/ljh/Downloads/libevent/build/include)

target_link_libraries(client PUBLIC      event_core)
target_link_directories(client PUBLIC    /home/ljh/Downloads/libevent/build/lib)
target_include_directories(client PUBLIC /home/ljh/Downloads/libevent/include
                                         /home/ljh/Downloads/libevent/build/include)

#add_subdirectory(./foo)

# make DESTDIR=~/myproject install
install(TARGETS server client # foo
    )

file(GLOB LIBEVENT_LIB /home/ljh/Downloads/libevent/build/lib/*)
install(FILES ${LIBEVENT_LIB} DESTINATION lib
    )
