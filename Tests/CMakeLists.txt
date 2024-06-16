# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

cmake_minimum_required(VERSION 3.18)
project(Aleph-w)

set(CMAKE_CXX_STANDARD 20)

link_directories(..)
include_directories(..)

find_package(GTest REQUIRED)

add_executable(array_utils array_utils.cc)
add_executable(hash  hash.cc)
add_executable(aleph-graph  aleph-graph.cc)
add_executable(hash-dry  hash-dry.cc)

