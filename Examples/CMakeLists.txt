cmake_minimum_required(VERSION 3.10)

project(Examples)

set(CMAKE_CXX_STANDARD 20)

set(source_files btreepic.C deway.C evalExp.C
        fibonacci.C generate_forest.C
        gen_rand_graph.C graphpic.C joseph.C ntreepic.C
        polinom.C test_2dtree.C test_agraph.C test-comb.C)

foreach (source_file ${source_files})
    get_filename_component(target_name ${source_file} NAME_WE)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/..)
    target_link_directories(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/..)
    target_link_libraries(${target_name} Aleph gmpxx gmp asprintf mpfr gsl gslcblas m X11 c)
endforeach (source_file)

