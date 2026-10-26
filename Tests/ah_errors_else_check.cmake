# An else written right after a conditional macro of ah-errors.H,
# parse_utils.H or ah-trace.H, with no if around it, must not compile: the
# macros leave no if open for it to belong to. For each macro, a function
# with that else must fail to compile, on the line of the else; and a
# function with all the macros, without it, must compile.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P ah_errors_else_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

# Each entry: the header that defines the macro, a colon, and a call
set(CALLS
  "ah-errors.H:ah_range_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_range_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_runtime_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_runtime_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_logic_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_logic_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_underflow_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_underflow_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_overflow_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_overflow_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_domain_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_domain_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_out_of_range_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_out_of_range_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_invalid_argument_if(x < 0) << \"m\""
  "ah-errors.H:ah_invalid_argument_unless(x < 0) << \"m\""
  "ah-errors.H:ah_length_error_if(x < 0) << \"m\""
  "ah-errors.H:ah_length_error_unless(x < 0) << \"m\""
  "ah-errors.H:ah_bad_alloc_if(x < 0)"
  "ah-errors.H:ah_bad_alloc_unless(x < 0)"
  "ah-errors.H:ah_range_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_range_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_runtime_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_runtime_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_logic_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_logic_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_underflow_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_underflow_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_overflow_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_overflow_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_domain_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_domain_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_out_of_range_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_out_of_range_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_invalid_argument_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_invalid_argument_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_length_error_if_constexpr(sizeof(int) == 0) << \"m\""
  "ah-errors.H:ah_length_error_unless_constexpr(sizeof(int) > 0) << \"m\""
  "ah-errors.H:ah_bad_alloc_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_bad_alloc_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_warning_if(std::cerr, x < 0) << \"m\""
  "ah-errors.H:ah_warning_unless(std::cerr, x < 0) << \"m\""
  "parse_utils.H:ah_parse_error_if(x < 0) << \"m\""
  "parse_utils.H:ah_parse_error_unless(x < 0) << \"m\""
  "ah-trace.H:ah_trace_if(std::cerr, x < 0) << \"m\""
  "ah-trace.H:ah_trace_unless(std::cerr, x < 0) << \"m\""
  "ah-trace.H:ah_msg_if(std::cerr, x < 0) << \"m\""
  "ah-trace.H:ah_msg_unless(std::cerr, x < 0) << \"m\""
)

set(FLAGS "-std=c++20" "-fsyntax-only" "-I${SOURCE_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")

set(all_includes "")
set(all_calls "")
set(failures "")
set(index 0)
foreach(entry IN LISTS CALLS)
  string(FIND "${entry}" ":" colon)
  string(SUBSTRING "${entry}" 0 ${colon} header)
  math(EXPR start "${colon} + 1")
  string(SUBSTRING "${entry}" ${start} -1 call)

  # The else is on line 6
  set(source "${WORK_DIR}/else_after_call_${index}.cc")
  file(WRITE "${source}"
    "#include <${header}>\n"
    "#include <iostream>\n"
    "int f(int x)\n"
    "{\n"
    "  ${call};\n"
    "  else\n"
    "    return 1;\n"
    "  return 0;\n"
    "}\n")
  execute_process(
    COMMAND "${CXX_COMPILER}" ${FLAGS} "${source}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  string(REGEX MATCH "[^\n]*: error:[^\n]*" first_error "${output}")
  if(result EQUAL 0)
    list(APPEND failures "${call}: the else after it compiles")
  elseif(NOT first_error MATCHES "else_after_call_${index}\\.cc:6:")
    list(APPEND failures
      "${call}: the first error is not on the line of the else: ${first_error}")
  endif()

  string(APPEND all_includes "#include <${header}>\n")
  string(APPEND all_calls "  ${call};\n")
  math(EXPR index "${index} + 1")
endforeach()

# Without the else, every call compiles
set(source "${WORK_DIR}/calls_without_else.cc")
file(WRITE "${source}"
  "${all_includes}#include <iostream>\nint f(int x)\n{\n${all_calls}  return 0;\n}\n")
execute_process(
  COMMAND "${CXX_COMPILER}" ${FLAGS} "${source}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  list(APPEND failures "the calls without an else do not compile:\n${output}")
endif()

if(failures)
  string(REPLACE ";" "\n" failures "${failures}")
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "${index} calls checked")
