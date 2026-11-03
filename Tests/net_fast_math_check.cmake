# The floating flows of tpl_net.H sum the flows of a node keeping what
# their rounding loses (Flow_Sum), which needs strict floating arithmetic:
# with -ffast-math a unit that runs a floating preflow must not compile, by
# the static_assert of Flow_Sum. Its controls compile: the same unit without
# -ffast-math, and a preflow with integer flows with it.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P net_fast_math_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

set(FLAGS "-std=c++20" "-fsyntax-only" "-I${SOURCE_DIR}")

# On macOS, Homebrew installs GSL and GMP outside the default search path
# of the compiler, as in ah_errors_else_check.cmake
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE BREW_RC
    ERROR_QUIET
  )
  if(BREW_RC EQUAL 0 AND EXISTS "${BREW_PREFIX}/include")
    list(APPEND FLAGS "-I${BREW_PREFIX}/include")
  endif()
endif()

file(MAKE_DIRECTORY "${WORK_DIR}")

# Compiles a unit that runs the FIFO preflow of tpl_net.H with flows of type
# flow, with the options extra, and gives back the result of the compiler
# and its output
function(compile_preflow name flow extra out_result out_output)
  set(source "${WORK_DIR}/${name}.cc")
  file(WRITE "${source}"
    "#include <tpl_net.H>\n"
    "using Net = Aleph::Net_Graph<Aleph::Net_Node<int>, Aleph::Net_Arc<int, ${flow}>>;\n"
    "${flow} f(Net & net) { return Aleph::fifo_preflow_maximum_flow(net); }\n")
  execute_process(
    COMMAND "${CXX_COMPILER}" ${FLAGS} ${extra} "${source}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  set(${out_result} "${result}" PARENT_SCOPE)
  set(${out_output} "${output}" PARENT_SCOPE)
endfunction()

set(failures "")

compile_preflow(double_fast_math double "-ffast-math" result output)
if(result EQUAL 0)
  list(APPEND failures "a preflow with double flows compiles with -ffast-math")
elseif(NOT output MATCHES "need strict floating")
  list(APPEND failures
    "a preflow with double flows fails with -ffast-math, but not by the static_assert of Flow_Sum:\n${output}")
endif()

compile_preflow(double_strict double "" result output)
if(NOT result EQUAL 0)
  list(APPEND failures "a preflow with double flows does not compile without -ffast-math:\n${output}")
endif()

compile_preflow(int_fast_math int "-ffast-math" result output)
if(NOT result EQUAL 0)
  list(APPEND failures "a preflow with int flows does not compile with -ffast-math:\n${output}")
endif()

if(failures)
  string(REPLACE ";" "\n" failures "${failures}")
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "a floating preflow needs strict floating arithmetic: 3 units checked")
