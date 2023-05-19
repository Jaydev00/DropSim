cmake_minimum_required(VERSION 3.15)

project(DropSimulation VERSION 1.0)

configure_file(versions.h.in versions.h)
add_executable(DropSimulation Main.cpp 
IO.cpp
ReportThreadData.cpp
SimResult.cpp
SimArgs.cpp
ThreadData.cpp
Simulation.cpp)
target_include_directories(DropSimulation PUBLIC "${PROJECT_BINARY_DIR}")



add_library(DropSimulation_compiler_flags INTERFACE)

#TODO make compiler flags generic
target_compile_features(DropSimulation_compiler_flags INTERFACE cxx_std_14)

find_package (Threads)
target_link_libraries (DropSimulation ${CMAKE_THREAD_LIBS_INIT})
