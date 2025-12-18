cmake_minimum_required(VERSION 3.18)

project(Aleph-Tests CXX)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

# Debug flags with sanitizers (optional)
option(USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
if(USE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

# Include parent directory for Aleph headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT TARGET Aleph)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/aleph)
endif()

# Link directory for Aleph library
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# When this directory is configured standalone (without the root
# CMakeLists setting ALEPH_SYSTEM_LIBS), define the same defaults here so
# that tests depending on Aleph's transitive libraries link correctly.
if(NOT DEFINED ALEPH_SYSTEM_LIBS)
    set(ALEPH_SYSTEM_LIBS
        gmpxx
        gmp
        pthread
        mpfr
        gsl
        gslcblas
        m
        X11
        Threads::Threads)
endif()

# Find Google Test
find_package(GTest REQUIRED)

# GoogleMock is not always installed alongside GTest. Some tests (e.g.
# ringfilecache.cc) require gmock headers. Detect availability and skip those
# targets if gmock is missing.
set(ALEPH_HAVE_GMOCK OFF)
if(TARGET GTest::gmock)
    set(ALEPH_HAVE_GMOCK ON)
elseif(EXISTS "/usr/include/gmock/gmock.h")
    set(ALEPH_HAVE_GMOCK ON)
elseif(DEFINED ENV{GOOGLETEST} AND EXISTS "$ENV{GOOGLETEST}/googlemock/include/gmock/gmock.h")
    set(ALEPH_HAVE_GMOCK ON)
endif()

# Note: You may need to set GOOGLETEST environment variable or specify the path
# export GOOGLETEST=/path/to/googletest
# or set it in CMake: cmake -DGOOGLETEST=/path/to/googletest ..

if(DEFINED ENV{GOOGLETEST})
    include_directories($ENV{GOOGLETEST}/googletest/include)
    include_directories($ENV{GOOGLETEST}/googlemock/include)
endif()

# List of all test programs from Imakefile
set(TEST_PROGRAMS
    array
    array_utils
    array-it
    bloom-filter
    dynarray
    union
    sort_utils
    memarray
    slinknc
    snodenc
    htlist
    dynlist
    dlink
    dnode
    dyndlist
    arraystack
    fixedstack
    dynliststack
    arrayqueue
    dynlistqueue
    bin-node
    bin-node-utils
    bin-node-xt
    splay-tree
    splay-tree-rk
    rb-tree
    dynsettree
    dynsethash
    rand-tree
    lin-hash
    bintree
    avl
    binheap
    tree-node
    odhash
    dynsetohash
    hash-it
    ah-zip
    ah-functional
    ah-dry
    tree-it
    al-domain
    al-vector
    al-matrix
    # ah-arena  # TODO: DynSetTree missing arena_allocated_size/arena_available_size
    ah-string-utils
    ah-utils
    index-node
    # sort_arrays  # TODO: to_vector not declared
    primes
    generate_tree
    fixedqueue
    randomqueue
    slink
    snode
    slist
    dynslist
    arrayheap
    arrayheap_algos
    dynarrayheap
    ringfilecache
    tpl_indexArc_test
    tpl_dynarray_set_test
    random_graph
    tarjan_test
)

# Exhaustive tests (files ending in _test.cc)
set(EXHAUSTIVE_TESTS
    dlink_test
    htlist_test
)

# Create executable for each test program
foreach(test_program ${TEST_PROGRAMS})
    # Check if source file exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_program}.cc")
        add_executable(${test_program} ${test_program}.cc)

        # Link with Aleph library and system libraries
        target_link_libraries(${test_program} PRIVATE
            Aleph
            GTest::gtest_main
            GTest::gtest
            ${ALEPH_SYSTEM_LIBS}
        )

        # Add test to CTest
        add_test(NAME ${test_program} COMMAND ${test_program})

        # Optional: Install test executable
        # install(TARGETS ${test_program} DESTINATION bin/tests)
    else()
        message(WARNING "Source file not found for test: ${test_program}.cc")
    endif()
endforeach()

# Create executable for each exhaustive test program (files ending in _test.cc)
# These tests are for header-only classes and need minimal dependencies
foreach(test_program ${EXHAUSTIVE_TESTS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_program}.cc")
        add_executable(${test_program} ${test_program}.cc)

        # Header-only tests only need gtest and pthread
        target_link_libraries(${test_program} PRIVATE
            GTest::gtest_main
            GTest::gtest
            Threads::Threads
        )

        add_test(NAME ${test_program} COMMAND ${test_program})
    else()
        message(WARNING "Source file not found for exhaustive test: ${test_program}.cc")
    endif()
endforeach()

# Enable CTest
enable_testing()

message(STATUS "Configured ${CMAKE_PROJECT_NAME} with Google Test")
message(STATUS "  Standard test programs: ${TEST_PROGRAMS}")
message(STATUS "  Exhaustive test programs: ${EXHAUSTIVE_TESTS}")
message(STATUS "  Use 'make test' or 'ctest' to run all tests")
message(STATUS "  Use 'ctest -R _test' to run only exhaustive tests")
