cmake_minimum_required(VERSION 3.16)
project("PG60 Manager" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(PG60Manager
    src/main.cpp
    src/MainWindow.cpp
    src/Player.cpp
    src/PlayerList.cpp
    src/EditPlayerDialog.cpp
)

target_link_libraries(PG60Manager PRIVATE Qt6::Widgets)
