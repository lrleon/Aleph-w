# The conditional macros take, as their condition, what if takes, but for a
# declaration: what converts to bool, even explicitly, and not a scoped
# enumeration. Each kind of condition is given to each form of macro.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P ah_errors_condition_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

set(FLAGS "-std=c++20" "-fsyntax-only" "-I${SOURCE_DIR}")

# On macOS, Homebrew installs GSL outside the default search path of the
# compiler, and parse_utils.H includes <gsl/gsl_rng.h> (through
# ah-string-utils.H, ah-zip.H and tpl_dynSkipList.H). As
# ah_errors_else_check.cmake, add the include directory of Homebrew.
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

set(PRELUDE
"#include <ah-errors.H>
#include <ah-trace.H>
#include <parse_utils.H>
#include <iostream>
#include <optional>
enum class Scoped { off, on };
enum Plain : unsigned { plain_off = 0, plain_on = 2 };
struct Explicit { bool v; constexpr explicit operator bool() const noexcept { return v; } };
int * find_it(int);
static int obj;
")

# The forms; @ is the condition
set(FORMS
  "ah_domain_error_if(@) << \"m\""
  "ah_domain_error_unless(@) << \"m\""
  "ah_parse_error_if(@) << \"m\""
  "ah_parse_error_unless(@) << \"m\""
  "ah_bad_alloc_if(@)"
  "ah_bad_alloc_unless(@)"
  "ah_warning_if(std::cerr, @) << \"m\""
  "ah_warning_unless(std::cerr, @) << \"m\""
  "ah_trace_if(std::cerr, @) << \"m\""
  "ah_trace_unless(std::cerr, @) << \"m\""
  "ah_msg_if(std::cerr, @) << \"m\""
  "ah_msg_unless(std::cerr, @) << \"m\""
)
set(CONSTEXPR_FORMS
  "ah_domain_error_if_constexpr(@) << \"m\""
  "ah_domain_error_unless_constexpr(@) << \"m\""
  "ah_bad_alloc_if_constexpr(@)"
  "ah_bad_alloc_unless_constexpr(@)"
)
# What converts to bool, at run time and as a constant
set(TAKEN b i p o e u)
set(TAKEN_CONSTANT "false" "2" "(plain_on & 1u)" "Explicit{false}" "(&obj == nullptr)")
# What does not
set(REJECTED "s" "auto q = find_it(x)")
set(REJECTED_CONSTANT "Scoped::off")

set(failures "")

# Every form takes every condition that converts to bool
set(body "")
foreach(form IN LISTS FORMS)
  foreach(c IN LISTS TAKEN)
    string(REPLACE "@" "${c}" call "${form}")
    string(APPEND body "  ${call};\n")
  endforeach()
endforeach()
foreach(form IN LISTS CONSTEXPR_FORMS)
  foreach(c IN LISTS TAKEN_CONSTANT)
    string(REPLACE "@" "${c}" call "${form}")
    string(APPEND body "  ${call};\n")
  endforeach()
endforeach()
file(WRITE "${WORK_DIR}/taken.cc"
  "${PRELUDE}int f(int x, Scoped s)\n{\n  bool b = x > 0; int i = x; int * p = find_it(x);\n  std::optional<int> o; Explicit e{b}; Plain u = plain_on;\n  (void) s;\n${body}  return 0;\n}\n")
execute_process(COMMAND "${CXX_COMPILER}" ${FLAGS} "${WORK_DIR}/taken.cc"
  RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
if(NOT result EQUAL 0)
  list(APPEND failures "the conditions that convert to bool do not compile:\n${output}")
endif()

# No form takes a scoped enumeration nor a declaration; the error is on the
# line of the call, which is line 14
set(index 0)
foreach(group "FORMS:REJECTED" "CONSTEXPR_FORMS:REJECTED_CONSTANT")
  string(REPLACE ":" ";" pair "${group}")
  list(GET pair 0 forms_var)
  list(GET pair 1 conds_var)
  foreach(form IN LISTS ${forms_var})
    foreach(c IN LISTS ${conds_var})
      string(REPLACE "@" "${c}" call "${form}")
      set(source "${WORK_DIR}/rejected_${index}.cc")
      file(WRITE "${source}"
        "${PRELUDE}int f(int x, Scoped s)\n{\n  (void) s;\n  ${call};\n  return x;\n}\n")
      execute_process(COMMAND "${CXX_COMPILER}" ${FLAGS} "${source}"
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
      if(result EQUAL 0)
        list(APPEND failures "${call}: compiles")
      elseif(NOT output MATCHES "rejected_${index}\\.cc:14:")
        string(REGEX MATCH "[^\n]*: error:[^\n]*" first_error "${output}")
        list(APPEND failures "${call}: the error is not on the line of the call: ${first_error}")
      endif()
      math(EXPR index "${index} + 1")
    endforeach()
  endforeach()
endforeach()

if(failures)
  string(REPLACE ";" "\n" failures "${failures}")
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "${index} conditions rejected")
