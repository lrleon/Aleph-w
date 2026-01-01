# Examples share configuration from the parent project; only add targets here

set(EXAMPLES_WARN_FLAGS "-Wall -Wextra -Wcast-align -Wno-sign-compare -Wno-write-strings -Wno-parentheses")
set(EXAMPLES_COMMON_FLAGS "-DMESSAGES -D_GLIBCXX__PTHREADS -D_REENTRANT -D__extern_always_inline=\"extern __always_inline\"")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXAMPLES_WARN_FLAGS} ${EXAMPLES_COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -O0 -g3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# List of all example programs present in the directory
set(EXAMPLE_PROGRAMS
    btreepic
    deway
    evalExp
    fib
    fib_stack
    fibonacci
    gen_rand_graph
    generate_forest
    graphpic
    hash_resize
    joseph
    ntreepic
    polinom
    timeAllTree
    writeAllTrees
    writeAvl
    writeBalance
    writeBinTree
    writeHeap
    writeInsertRoot
    writeJoin
    writeRandTree
    writeRankTree
    writeRb
    writeSplay
    writeSplit
    writeTreap
    write_floyd
    write_huffman
)

# Function to get source file from program name
function(get_source_file program_name out_var)
    # First try with .C extension
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${program_name}.C")
        set(${out_var} "${program_name}.C" PARENT_SCOPE)
    # Then try with .cc extension
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${program_name}.cc")
        set(${out_var} "${program_name}.cc" PARENT_SCOPE)
    else()
        string(REPLACE "-" "_" alt_name ${program_name})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${alt_name}.C")
            set(${out_var} "${alt_name}.C" PARENT_SCOPE)
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${alt_name}.cc")
            set(${out_var} "${alt_name}.cc" PARENT_SCOPE)
        else()
            set(${out_var} "" PARENT_SCOPE)
        endif()
    endif()
endfunction()

# Create executable for each example program
foreach(program ${EXAMPLE_PROGRAMS})
    get_source_file(${program} source_file)

    if(source_file)
        add_executable(${program} ${source_file})

        target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
    else()
        message(WARNING "Source file not found for program: ${program}")
    endif()
endforeach()

message(STATUS "Configured ${CMAKE_PROJECT_NAME} examples")
