cmake_minimum_required(VERSION 3.21)
project(MusicProgram VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
enable_testing()

add_library(musiccore STATIC src/Program.cpp src/Program.h src/Storage.cpp src/Storage.h)
target_include_directories(musiccore PUBLIC src)
target_link_libraries(musiccore PUBLIC Qt6::Core)

if(MSVC)
  target_compile_options(musiccore PUBLIC /W4)
else()
  target_compile_options(musiccore PUBLIC -Wall -Wextra -Wpedantic)
endif()

qt_add_executable(music_app src/main.cpp src/MainWindow.cpp src/MainWindow.h)
target_link_libraries(music_app PRIVATE musiccore Qt6::Widgets)

qt_add_executable(tst_music tests/tst_music.cpp)
target_link_libraries(tst_music PRIVATE musiccore Qt6::Test)
add_test(NAME tst_music COMMAND tst_music)
