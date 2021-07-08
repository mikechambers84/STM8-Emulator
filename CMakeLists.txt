cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

project(stm8-emulator)
include(CMakePrintHelpers)
find_package(Curses REQUIRED)
file(GLOB_RECURSE SOURCES "STM8/*.c")
set(HEADERS "STM8" "STM8/peripherals" "STM8/hardware" "STM8/devices")

cmake_print_variables(${CURSES_INCLUDE_DIRS})

add_executable(STM8_LINUX ${SOURCES})
target_include_directories(STM8_LINUX PRIVATE ${HEADERS})
target_include_directories(STM8_LINUX PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(STM8_LINUX PRIVATE ${CURSES_LIBRARIES})