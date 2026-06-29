# Phase 26 — libFuzzer targets for the untrusted-input parsers.
#
# Opt-in and Clang-only (libFuzzer ships with Clang). Enable with:
#
#   cmake -S . -B build-fuzz -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
#         -DALEPH_BUILD_FUZZERS=ON -DBUILD_TESTS=ON
#   cmake --build build-fuzz --target fuzz_rle_parser
#   ./build-fuzz/Tests/fuzz/fuzz_rle_parser -max_total_time=60 \
#       Tests/fuzz/corpus/rle
#
# Each target is built with -fsanitize=fuzzer,address,undefined. The parsers
# are header-only, so they are instrumented directly into the fuzzer TU; the
# checkpoint loader additionally exercises the bundled miniz through ASan's
# global malloc interceptor.
option(ALEPH_BUILD_FUZZERS "Build libFuzzer targets (Clang only)" OFF)

if(ALEPH_BUILD_FUZZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
      "ALEPH_BUILD_FUZZERS=ON requires Clang (libFuzzer); "
      "current compiler is ${CMAKE_CXX_COMPILER_ID}.")
  endif()

  set(_aleph_fuzz_sanitizers "fuzzer,address,undefined")

  set(_aleph_fuzz_targets
    fuzz_rle_parser
    fuzz_life_parser
    fuzz_csv_reader
    fuzz_checkpoint_loader
  )

  foreach(_t ${_aleph_fuzz_targets})
    add_executable(${_t} ${_t}.cc)
    # htlist.H requires -fno-strict-aliasing; keep it for the fuzzer TU too.
    target_compile_options(${_t} PRIVATE
      -g -O1 -fno-strict-aliasing
      -fno-sanitize-recover=undefined
      -fsanitize=${_aleph_fuzz_sanitizers})
    target_link_options(${_t} PRIVATE
      -fsanitize=${_aleph_fuzz_sanitizers})
    target_link_libraries(${_t} PRIVATE Aleph ${ALEPH_SYSTEM_LIBS})
    set_target_properties(${_t} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endforeach()

  message(STATUS "Aleph fuzzers enabled: ${_aleph_fuzz_targets}")
endif()