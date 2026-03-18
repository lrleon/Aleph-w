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

list(LENGTH EXAMPLE_SOURCES _examples_count)
message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples (${_examples_count} targets)")
