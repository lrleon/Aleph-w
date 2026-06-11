cmake_minimum_required(VERSION 3.18)

set(CA_REPRODUCTION_PROGRAMS
    btw_sandpile
    schelling_segregation
    drossel_schwabl
    gray_scott_patterns
    game_of_life_bench
)

foreach(program IN LISTS CA_REPRODUCTION_PROGRAMS)
    add_executable(${program} "${program}.cc")
    target_link_libraries(${program} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
    target_compile_definitions(${program} PRIVATE
        ALEPH_REPRODUCTIONS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()

add_custom_target(ca_reproductions
    DEPENDS ${CA_REPRODUCTION_PROGRAMS}
)

message(STATUS "Configured cellular-automata reproductions")
