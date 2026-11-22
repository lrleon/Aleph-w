# Runs the visual example of adaptive Huffman coding on one message and
# checks the page that it writes: its exit status alone says nothing of it.
#
#   EXAMPLE_PATH  the example
#   OUTPUT_PATH   the page that it writes
#   MODE          empty: the empty message, whose page has its initial frame
#                 alone; escaped: a message of markup characters, which the
#                 page must have as text and never as markup

foreach(_required EXAMPLE_PATH OUTPUT_PATH MODE)
  if(NOT DEFINED ${_required})
    message(FATAL_ERROR "${_required} is required")
  endif()
endforeach()

if(MODE STREQUAL "empty")
  set(_text "")
  set(_frames 1)
  set(_expected "data-length='0'" "Empty message")
  set(_rejected "data-step='1'")
elseif(MODE STREQUAL "escaped")
  set(_text "<A&\"'B>")
  set(_frames 8)
  # A semicolon would split a list: each entity is checked by itself
  set(_expected "&lt\;A&amp\;&quot\;&#39\;B&gt\;" ">&lt\;</button>" ">&amp\;</button>"
      ">&quot\;</button>" ">&#39\;</button>" ">&gt\;</button>" "data-length='7'")
  set(_rejected "<A&" "&\"'" "'B>")
else()
  message(FATAL_ERROR "MODE must be empty or escaped, not '${MODE}'")
endif()

file(REMOVE "${OUTPUT_PATH}")
execute_process(
  COMMAND "${EXAMPLE_PATH}" "${OUTPUT_PATH}" "${_text}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _stdout
  ERROR_VARIABLE _stderr
)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR
    "The example failed with exit code ${_result}\n"
    "stdout:\n${_stdout}\n"
    "stderr:\n${_stderr}")
endif()
if(NOT EXISTS "${OUTPUT_PATH}")
  message(FATAL_ERROR "The example did not write ${OUTPUT_PATH}")
endif()

file(READ "${OUTPUT_PATH}" _page)

string(REGEX MATCHALL "<article class='frame'" _found "${_page}")
list(LENGTH _found _count)
if(NOT _count EQUAL _frames)
  message(FATAL_ERROR "The page has ${_count} frames, not ${_frames}")
endif()

foreach(_item IN LISTS _expected)
  string(FIND "${_page}" "${_item}" _pos)
  if(_pos EQUAL -1)
    message(FATAL_ERROR "The page does not have: ${_item}")
  endif()
endforeach()

foreach(_item IN LISTS _rejected)
  string(FIND "${_page}" "${_item}" _pos)
  if(NOT _pos EQUAL -1)
    message(FATAL_ERROR "The page has, as markup: ${_item}")
  endif()
endforeach()
