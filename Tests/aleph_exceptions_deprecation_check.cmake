# aleph-exceptions.H is deprecated, and goes in 8.0.0. A program warns of
# it where it names its classes or uses its macros, and nowhere else: not
# where it includes the header, nor where it uses LFit, which throws
# SizeMismatch until 8.0.0. Each case is compiled with -Werror for the
# deprecations, and must compile or fail as it says.
#
# Run with: cmake -DCXX_COMPILER=<compiler> -DSOURCE_DIR=<repository>
#                 -DWORK_DIR=<directory> -P aleph_exceptions_deprecation_check.cmake

foreach(var CXX_COMPILER SOURCE_DIR WORK_DIR)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "${var} is required")
  endif()
endforeach()

set(FLAGS "-std=c++20" "-fsyntax-only" "-Wall" "-Wextra"
          "-Werror=deprecated-declarations" "-I${SOURCE_DIR}")

# On macOS, Homebrew installs GSL outside the default search path of the
# compiler, and lfit.H includes ah-random.H, which includes <gsl/gsl_rng.h>
# where it finds it. As ah_errors_condition_check.cmake, add the include
# directory of Homebrew, so that the check sees the headers that Aleph sees.
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

# What compiles without a warning
set(QUIET_NAMES include_it use_lfit)
set(QUIET_include_it
"#include <aleph-exceptions.H>
int main() { return 0; }
")
set(QUIET_use_lfit
"#include <lfit.H>
#include <exception>
using namespace Aleph;
int main()
{
  Array<double> x = {1, 2, 3}, y = {2, 4, 7}, e;
  LFit fit(x, y);
  try { LFit bad(e, e); } catch (const std::exception &) { return 0; }
  return 1;
}
")

# What warns
set(WARNS_NAMES name_a_class name_the_base define_one throw_one)
set(WARNS_name_a_class
"#include <lfit.H>
using namespace Aleph;
int main()
{
  try { Array<double> e; LFit f(e, e); } catch (const SizeMismatch &) { return 0; }
  return 1;
}
")
set(WARNS_name_the_base
"#include <aleph-exceptions.H>
int main() { try { throw 1; } catch (const AlephException &) {} return 0; }
")
set(WARNS_define_one
"#include <aleph-exceptions.H>
DEFINE_ALEPH_EXCEPTION(MyError, \"mine\")
int main() { return 0; }
")
set(WARNS_throw_one
"#include <aleph-exceptions.H>
int main() { try { ALEPHTHROW(InvalidValue, \"x\"); } catch (...) {} return 0; }
")

set(failures "")
foreach(name ${QUIET_NAMES})
  file(WRITE "${WORK_DIR}/${name}.cc" "${QUIET_${name}}")
  execute_process(COMMAND "${CXX_COMPILER}" ${FLAGS} "${WORK_DIR}/${name}.cc"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(NOT result EQUAL 0 OR output MATCHES "deprecated")
    string(APPEND failures "${name} must compile without a warning:\n${output}\n")
  endif()
endforeach()
foreach(name ${WARNS_NAMES})
  file(WRITE "${WORK_DIR}/${name}.cc" "${WARNS_${name}}")
  execute_process(COMMAND "${CXX_COMPILER}" ${FLAGS} "${WORK_DIR}/${name}.cc"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(result EQUAL 0 OR NOT output MATCHES "deprecated")
    string(APPEND failures "${name} must warn of the deprecation:\n${output}\n")
  endif()
endforeach()

if(failures)
  message(FATAL_ERROR "${failures}")
endif()
message(STATUS "aleph-exceptions.H warns where it is used, and only there")
