cmake_minimum_required(VERSION 3.16)
project(torrent-client CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# add_compile_options(-fsanitize=thread)
# add_link_options(-fsanitize=thread)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include(FetchContent)

set(CPR_USE_SYSTEM_CURL ON)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba) # The commit hash for 1.10.x. Replace with the latest from: https://github.com/libcpr/cpr/releases
FetchContent_MakeAvailable(cpr)


add_executable(
        ${PROJECT_NAME}
        src/main.cpp
        src/peer.h
        src/torrent_file.h
        src/peer_connect.cpp
        src/peer_connect.h
        src/tcp_connect.cpp
        src/tcp_connect.h
        src/torrent_tracker.cpp
        src/torrent_tracker.h
        src/torrent_file.cpp
        src/bencode.cpp
        src/bencode.h
        src/message.cpp
        src/message.h
        src/byte_tools.h
        src/byte_tools.cpp
        src/piece_storage.cpp
        src/piece_storage.h
        src/piece.cpp
        src/piece.h
        src/StaticThreadPool.cpp
        src/StaticThreadPool.h
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES} cpr::cpr)

