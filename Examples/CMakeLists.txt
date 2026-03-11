# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.C"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.CC"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

foreach(source_file IN LISTS EXAMPLE_SOURCES)
    get_filename_component(program "${source_file}" NAME_WE)
    string(REPLACE "-" "_" program "${program}")

    add_executable(${program} "${source_file}")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})

    get_filename_component(_ext "${source_file}" EXT)
    if(_ext STREQUAL ".c")
        set_target_properties(${program} PROPERTIES LINKER_LANGUAGE CXX)
    endif()
endforeach()

list(LENGTH EXAMPLE_SOURCES _examples_count)
message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples (${_examples_count} targets)")

