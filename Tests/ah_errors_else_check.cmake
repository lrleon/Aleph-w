# An else written right after a conditional macro of ah-errors.H,
# parse_utils.H or ah-trace.H, with no if around it, must not compile: the
# macros leave no if open for it to belong to. For each macro, a function
# with that else must fail to compile, on the line of the else; and a
# function with all the macros, without it, must compile.
#
# Neither must a message, given to a macro that throws, whose last << gives
# back void, as a << of the caller may after std::endl: it would not reach
# the operator that throws, and nothing would be thrown.
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

# On macOS, Homebrew installs GSL outside the default search path of the
# compiler, and parse_utils.H includes <gsl/gsl_rng.h> (through
# ah-string-utils.H, ah-zip.H and tpl_dynSkipList.H). As
# compiler_sdk_install_smoke.cmake, add the include directory of Homebrew.
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

# The macros that throw, and the header that defines each one
set(THROWING
  "ah-errors.H:ah_range_error_if(x < 0)"
  "ah-errors.H:ah_range_error_unless(x < 0)"
  "ah-errors.H:ah_range_error()"
  "ah-errors.H:ah_range_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_range_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_runtime_error_if(x < 0)"
  "ah-errors.H:ah_runtime_error_unless(x < 0)"
  "ah-errors.H:ah_runtime_error()"
  "ah-errors.H:ah_runtime_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_runtime_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_logic_error_if(x < 0)"
  "ah-errors.H:ah_logic_error_unless(x < 0)"
  "ah-errors.H:ah_logic_error()"
  "ah-errors.H:ah_logic_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_logic_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_underflow_error_if(x < 0)"
  "ah-errors.H:ah_underflow_error_unless(x < 0)"
  "ah-errors.H:ah_underflow_error()"
  "ah-errors.H:ah_underflow_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_underflow_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_overflow_error_if(x < 0)"
  "ah-errors.H:ah_overflow_error_unless(x < 0)"
  "ah-errors.H:ah_overflow_error()"
  "ah-errors.H:ah_overflow_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_overflow_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_domain_error_if(x < 0)"
  "ah-errors.H:ah_domain_error_unless(x < 0)"
  "ah-errors.H:ah_domain_error()"
  "ah-errors.H:ah_domain_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_domain_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_out_of_range_error_if(x < 0)"
  "ah-errors.H:ah_out_of_range_error_unless(x < 0)"
  "ah-errors.H:ah_out_of_range_error()"
  "ah-errors.H:ah_out_of_range_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_out_of_range_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_invalid_argument_if(x < 0)"
  "ah-errors.H:ah_invalid_argument_unless(x < 0)"
  "ah-errors.H:ah_invalid_argument()"
  "ah-errors.H:ah_invalid_argument_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_invalid_argument_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_length_error_if(x < 0)"
  "ah-errors.H:ah_length_error_unless(x < 0)"
  "ah-errors.H:ah_length_error()"
  "ah-errors.H:ah_length_error_if_constexpr(sizeof(int) == 0)"
  "ah-errors.H:ah_length_error_unless_constexpr(sizeof(int) > 0)"
  "ah-errors.H:ah_fatal_error()"
  "parse_utils.H:ah_parse_error_if(x < 0)"
  "parse_utils.H:ah_parse_error_unless(x < 0)"
  "parse_utils.H:ah_parse_error()"
)

# A message whose last << gives back void, after std::endl, must not
# compile, on the line of the message
set(void_index 0)
foreach(entry IN LISTS THROWING)
  string(FIND "${entry}" ":" colon)
  string(SUBSTRING "${entry}" 0 ${colon} header)
  math(EXPR start "${colon} + 1")
  string(SUBSTRING "${entry}" ${start} -1 call)

  # The message is on line 7
  set(source "${WORK_DIR}/void_tail_${void_index}.cc")
  file(WRITE "${source}"
    "#include <${header}>\n"
    "#include <iostream>\n"
    "struct Void_Tail {};\n"
    "void operator<<(std::ostream &, const Void_Tail &) {}\n"
    "int f(int x)\n"
    "{\n"
    "  ${call} << \"m\" << std::endl << Void_Tail{};\n"
    "  return x;\n"
    "}\n")
  execute_process(
    COMMAND "${CXX_COMPILER}" ${FLAGS} "${source}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE output)
  string(REGEX MATCH "[^\n]*: error:[^\n]*" first_error "${output}")
  if(result EQUAL 0)
    list(APPEND failures "${call}: a message that gives back void compiles")
  elseif(NOT output MATCHES "void_tail_${void_index}\\.cc:7:")
    # GCC gives the line of the macro, and that of the call in a note
    list(APPEND failures
      "${call}: the error is not on the line of the message: ${first_error}")
  endif()
  math(EXPR void_index "${void_index} + 1")
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
message(STATUS "${index} calls checked, ${void_index} with a message that gives back void")
