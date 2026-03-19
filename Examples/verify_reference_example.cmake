if(NOT DEFINED EXAMPLE_PATH)
  message(FATAL_ERROR "EXAMPLE_PATH is required")
endif()

if(NOT DEFINED EXPECT_SUBSTRINGS)
  message(FATAL_ERROR "EXPECT_SUBSTRINGS is required")
endif()

execute_process(
  COMMAND "${EXAMPLE_PATH}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)

if(NOT _result EQUAL 0)
  message(FATAL_ERROR
    "Example failed with exit code ${_result}\n"
    "stdout:\n${_stdout}\n"
    "stderr:\n${_stderr}")
endif()

set(_output "${_stdout}${_stderr}")

foreach(_expected IN LISTS EXPECT_SUBSTRINGS)
  string(FIND "${_output}" "${_expected}" _pos)
  if(_pos EQUAL -1)
    message(FATAL_ERROR
      "Expected substring not found: '${_expected}'\n"
      "Output was:\n${_output}")
  endif()
endforeach()
