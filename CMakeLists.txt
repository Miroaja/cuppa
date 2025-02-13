cmake_minimum_required(VERSION 3.5)
# If only we had tooling
project(terminal_particles)
find_package(Curses REQUIRED)

file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(cuppa ${sources})
target_link_libraries(cuppa ${CURSES_LIBRARIES})
target_compile_options(cuppa PUBLIC -std=c++20 -Wall -Werror -O3)
target_include_directories(cuppa PUBLIC src/)
install(TARGETS cuppa)
