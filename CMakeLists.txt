cmake_minimum_required(VERSION 3.5)
# If only we had tooling
project(terminal_particles)



file(GLOB_RECURSE sources src/*.cpp src/*.h)

add_executable(particles ${sources})
target_compile_options(particles PUBLIC -std=c++20 -Wall -Werror -O3)
target_include_directories(particles PUBLIC src/)



