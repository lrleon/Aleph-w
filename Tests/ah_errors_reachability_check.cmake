# What follows a macro of ah-errors.H or parse_utils.H that throws, when its
# condition is a constant that makes it throw, is unreachable, and the
# compiler must see it: a function that ends there needs no return, and a
# guard of a template protects what follows it. Each case must compile with
# -Wall -Wextra -Werror. GCC gives some of these warnings only when it
# generates code, so the cases are compiled with -c, not -fsyntax-only.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P ah_errors_reachability_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

execute_process(COMMAND "${CXX_COMPILER}" --version
  OUTPUT_VARIABLE version ERROR_VARIABLE version)
set(is_clang FALSE)
if(version MATCHES "clang")
  set(is_clang TRUE)
endif()

set(FLAGS "-std=c++20" "-Wall" "-Wextra" "-Werror" "-c" "-I${SOURCE_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
set(object "${WORK_DIR}/case.o")

# Each entry: the header, a colon, and a call whose condition makes it throw
set(THROWING
  "ah-errors.H:ah_range_error_if(true)"
  "ah-errors.H:ah_range_error_unless(false)"
  "ah-errors.H:ah_runtime_error_if(true)"
  "ah-errors.H:ah_runtime_error_unless(false)"
  "ah-errors.H:ah_logic_error_if(true)"
  "ah-errors.H:ah_logic_error_unless(false)"
  "ah-errors.H:ah_underflow_error_if(true)"
  "ah-errors.H:ah_underflow_error_unless(false)"
  "ah-errors.H:ah_overflow_error_if(true)"
  "ah-errors.H:ah_overflow_error_unless(false)"
  "ah-errors.H:ah_domain_error_if(true)"
  "ah-errors.H:ah_domain_error_unless(false)"
  "ah-errors.H:ah_out_of_range_error_if(true)"
  "ah-errors.H:ah_out_of_range_error_unless(false)"
  "ah-errors.H:ah_invalid_argument_if(true)"
  "ah-errors.H:ah_invalid_argument_unless(false)"
  "ah-errors.H:ah_length_error_if(true)"
  "ah-errors.H:ah_length_error_unless(false)"
  "ah-errors.H:ah_domain_error_if_constexpr(true)"
  "ah-errors.H:ah_domain_error_unless_constexpr(false)"
  "parse_utils.H:ah_parse_error_if(true)"
  "parse_utils.H:ah_parse_error_unless(false)"
)

set(failures "")
set(count 0)

function(check name source)
  file(WRITE "${WORK_DIR}/${name}.cc" "${source}")
  execute_process(
    COMMAND "${CXX_COMPILER}" ${FLAGS} "${WORK_DIR}/${name}.cc" -o "${object}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  if(NOT result EQUAL 0)
    string(REGEX MATCH "[^\n]*(warning|error):[^\n]*" first "${output}")
    set(failures "${failures};${name}: ${first}" PARENT_SCOPE)
  endif()
endfunction()

# A function that ends in the macro returns nothing after it
set(index 0)
foreach(entry IN LISTS THROWING)
  string(FIND "${entry}" ":" colon)
  string(SUBSTRING "${entry}" 0 ${colon} header)
  math(EXPR start "${colon} + 1")
  string(SUBSTRING "${entry}" ${start} -1 call)
  check("ends_in_${index}"
    "#include <${header}>\nint f(int k)\n{\n  if (k == 0)\n    return 1;\n  ${call} << \"k is \" << k;\n}\n")
  math(EXPR index "${index} + 1")
endforeach()
set(count ${index})

# A [[noreturn]] function that ends in the macro
check(noreturn
  "#include <ah-errors.H>\n[[noreturn]] void fail(const char * what)\n{\n  ah_runtime_error_if(true) << what;\n}\n")

# A branch that throws leaves a variable unset in the other
check(unset_in_the_branch_that_throws
  "#include <ah-errors.H>\nint f(int k)\n{\n  int v;\n  if (k == 0)\n    v = 1;\n  else\n    ah_domain_error_if(true) << k;\n  return v;\n}\n")

# A case that throws, followed by another
check(case_that_throws
  "#include <ah-errors.H>\nint f(int k)\n{\n  int r = 0;\n  switch (k)\n    {\n    case 0:\n      ah_domain_error_if(true) << k;\n    case 1:\n      r = 1;\n      break;\n    default:\n      r = 2;\n    }\n  return r;\n}\n")

# A guard of a template protects a division and an index
check(guarded_division
  "#include <ah-errors.H>\ntemplate <int N> int f(int x)\n{\n  ah_invalid_argument_if(N == 0) << \"N is 0\";\n  return x / N;\n}\ntemplate int f<0>(int);\n")
check(guarded_index
  "#include <ah-errors.H>\n#include <cstddef>\ntemplate <std::size_t I> int f()\n{\n  static constexpr int a[3] = {1, 2, 3};\n  ah_out_of_range_error_if(I >= 3) << I;\n  return a[I];\n}\ntemplate int f<5>();\n")

# GCC warns of the shift whatever guards it, as it did with if (C)
if(is_clang)
  check(guarded_shift
    "#include <ah-errors.H>\ntemplate <unsigned S> unsigned f(unsigned x)\n{\n  ah_domain_error_if(S >= 32) << S;\n  return x << S;\n}\ntemplate unsigned f<40>(unsigned);\n")
endif()

list(REMOVE_ITEM failures "")
if(failures)
  string(REPLACE ";" "\n" failures "${failures}")
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "${count} macros and the other cases compile without warnings")
