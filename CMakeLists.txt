cmake_minimum_required(VERSION 3.15)

project(DropSimulation Version 1.0)

add_library(DropSimulation_compiler_flags INTERFACE)
target_compile_features(DropSimulation_compiler_flags INTERFACE cxx_std_11)

