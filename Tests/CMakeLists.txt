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

link_directories(..)

add_executable(array_utils array_utils.cc)
target_link_libraries(array_utils Aleph GTest::GTest GTest::Main)

add_executable(array-it array-it.cc)

add_executable(ah-arena ah-arena.cc)
add_executable(ah-dry ah-dry.cc)
add_executable(ah-functional ah-functional.cc)
add_executable(ah-string-utils ah-string-utils.cc)
target_link_libraries(ah-string-utils PRIVATE GTest::gtest_main GTest::gtest)

#add_executable(ah-zip ah-zip.cc)

add_executable(al-domain al-domain.cc)
target_link_libraries(al-domain PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(al-matrix al-matrix.cc)
target_link_libraries(al-matrix PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(arrayqueue arrayqueue.cc)

add_executable(bin-node bin-node.cc)
target_link_libraries(bin-node PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(dlink dlink.cc)
target_link_libraries(dlink PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(dnode dnode.cc)
target_link_libraries(dnode PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(dynlist dynlist.cc)
target_link_libraries(dynlist PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(dynliststack dynliststack.cc)
target_link_libraries(dynliststack PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(dynlistqueue dynlistqueue.cc)
target_link_libraries(dynlistqueue PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(fixedqueue fixedqueue.cc)
target_link_libraries(fixedqueue PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(fixedstack fixedstack.cc)
target_link_libraries(fixedstack PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(generate_tree generate_tree.cc)
target_link_libraries(generate_tree PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

add_executable(hash-it hash-it.cc)
target_link_libraries(hash-it PRIVATE Aleph
        GTest::gtest_main GTest::gtest
gsl)

