# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Use glob-based discovery for EXAMPLE_SOURCES instead of a hardcoded list
# to improve maintainability. The CONFIGURE_DEPENDS flag enables automatic
# re-configuration of the build system when new files are added or removed
# from the directory. Note that while this simplifies adding new examples,
# it may have minor performance impacts on some build systems and behavior
# can be platform-dependent.
file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.C"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.CC"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

# Exclude known non-standalone or helper files that should not be built 
# as individual executables.
set(EXCLUDED_EXAMPLES
    "${CMAKE_CURRENT_SOURCE_DIR}/fft_benchmark.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/fft_reference_probe.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/polynomial_benchmark.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/polynomial_reference_probe.cc"
)

foreach(excluded_file IN LISTS EXCLUDED_EXAMPLES)
    list(REMOVE_ITEM EXAMPLE_SOURCES "${excluded_file}")
endforeach()

foreach(source_file IN LISTS EXAMPLE_SOURCES)
    get_filename_component(program "${source_file}" NAME_WE)

    add_executable(${program} "${source_file}")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})

    get_filename_component(_ext "${source_file}" EXT)
    if(_ext STREQUAL ".c")
        set_source_files_properties("${source_file}" PROPERTIES LANGUAGE CXX)
    endif()
endforeach()

add_executable(polynomial_benchmark "${CMAKE_CURRENT_SOURCE_DIR}/polynomial_benchmark.cc")
target_link_libraries(polynomial_benchmark PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})

add_executable(polynomial_reference_probe "${CMAKE_CURRENT_SOURCE_DIR}/polynomial_reference_probe.cc")
target_link_libraries(polynomial_reference_probe PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})

if(BUILD_TESTS)
    set(_VERIFY_REFERENCE_EXAMPLE
        ${CMAKE_CURRENT_SOURCE_DIR}/verify_reference_example.cmake)

    if(TARGET backtracking_nqueens_example)
        add_test(
            NAME StateSearchExamples.NQueens
            COMMAND ${CMAKE_COMMAND}
                    -DEXAMPLE_PATH=$<TARGET_FILE:backtracking_nqueens_example>
                    "-DEXPECT_SUBSTRINGS=N-Queens reference example;Solutions found: 2;First solution signature: 1302"
                    -P ${_VERIFY_REFERENCE_EXAMPLE}
        )
    endif()

    if(TARGET branch_and_bound_knapsack_example)
        add_test(
            NAME StateSearchExamples.Knapsack
            COMMAND ${CMAKE_COMMAND}
                    -DEXAMPLE_PATH=$<TARGET_FILE:branch_and_bound_knapsack_example>
                    "-DEXPECT_SUBSTRINGS=0/1 Knapsack reference example;Optimal value: 90;Chosen items: 0 2"
                    -P ${_VERIFY_REFERENCE_EXAMPLE}
        )
    endif()

    if(TARGET negamax_tictactoe_example)
        add_test(
            NAME StateSearchExamples.TicTacToe
            COMMAND ${CMAKE_COMMAND}
                    -DEXAMPLE_PATH=$<TARGET_FILE:negamax_tictactoe_example>
                    "-DEXPECT_SUBSTRINGS=Tic-Tac-Toe reference example;Negamax best move: 4;Alpha-Beta best move: 4"
                    -P ${_VERIFY_REFERENCE_EXAMPLE}
        )
    endif()

    if(TARGET state_search_benchmark)
        add_test(
            NAME StateSearchBenchmark.Validate
            COMMAND $<TARGET_FILE:state_search_benchmark> --validate
        )
    endif()
endif()

list(LENGTH EXAMPLE_SOURCES _examples_count)
math(EXPR _total_examples "${_examples_count} + 2")
message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples (${_total_examples} targets)")
