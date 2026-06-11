if(NOT DEFINED BUILD_DIR)
  message(FATAL_ERROR "BUILD_DIR is required")
endif()

if(NOT DEFINED CXX_COMPILER)
  message(FATAL_ERROR "CXX_COMPILER is required")
endif()

if(NOT DEFINED SMOKE_SOURCE)
  message(FATAL_ERROR "SMOKE_SOURCE is required")
endif()

set(SMOKE_PREFIX "${BUILD_DIR}/sdk-install-smoke-prefix")
set(SMOKE_OBJECT "${BUILD_DIR}/sdk-install-smoke.o")

file(REMOVE_RECURSE "${SMOKE_PREFIX}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${SMOKE_PREFIX}"
  RESULT_VARIABLE install_result
  OUTPUT_VARIABLE install_stdout
  ERROR_VARIABLE install_stderr
)

if(NOT install_result EQUAL 0)
  message(FATAL_ERROR
    "Failed to install Aleph-w into smoke prefix.\n"
    "stdout:\n${install_stdout}\n"
    "stderr:\n${install_stderr}\n")
endif()

set(SMOKE_COMPILE_FLAGS
    "-std=c++20"
    "-I${SMOKE_PREFIX}/include/aleph"
)

# On macOS, Homebrew installs GMP/MPFR/GSL outside the default compiler
# search path (e.g. /opt/homebrew/include on Apple Silicon, /usr/local on
# Intel). The installed SDK headers transitively #include <gsl/gsl_rng.h>
# (via ah-zip.H → tpl_dynSkipList.H), so the smoke compile fails unless we
# also add the Homebrew include prefix. Mirrors the logic in the top-level
# CMakeLists.txt that adds the same path to the Aleph target.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
  execute_process(
    COMMAND brew --prefix
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE BREW_RC
    ERROR_QUIET
  )
  if(BREW_RC EQUAL 0 AND EXISTS "${BREW_PREFIX}/include")
    list(APPEND SMOKE_COMPILE_FLAGS "-I${BREW_PREFIX}/include")
  endif()
endif()

execute_process(
  COMMAND "${CXX_COMPILER}"
          ${SMOKE_COMPILE_FLAGS}
          "-c"
          "${SMOKE_SOURCE}"
          "-o"
          "${SMOKE_OBJECT}"
  RESULT_VARIABLE compile_result
  OUTPUT_VARIABLE compile_stdout
  ERROR_VARIABLE compile_stderr
)

if(NOT compile_result EQUAL 0)
  message(FATAL_ERROR
    "Installed SDK header smoke compile failed.\n"
    "stdout:\n${compile_stdout}\n"
    "stderr:\n${compile_stderr}\n")
endif()
