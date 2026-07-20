if(NOT DEFINED EXAMPLE_PATH OR NOT DEFINED PDFLATEX_PATH OR
   NOT DEFINED OUTPUT_DIR OR NOT DEFINED MODE)
    message(FATAL_ERROR
        "EXAMPLE_PATH, PDFLATEX_PATH, OUTPUT_DIR, and MODE are required")
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(_tex_path "${OUTPUT_DIR}/document.tex")

set(_example_command "${EXAMPLE_PATH}" "${_tex_path}")
if(MODE STREQUAL "handout")
    list(APPEND _example_command --handout)
elseif(NOT MODE STREQUAL "standalone" AND NOT MODE STREQUAL "beamer")
    message(FATAL_ERROR "Unsupported LaTeX example mode: ${MODE}")
endif()

execute_process(
    COMMAND ${_example_command}
    TIMEOUT 60
    RESULT_VARIABLE _example_status
    OUTPUT_VARIABLE _example_stdout
    ERROR_VARIABLE _example_stderr
)
if(NOT _example_status EQUAL 0)
    message(FATAL_ERROR
        "Example failed with status ${_example_status}:\n"
        "${_example_stdout}${_example_stderr}")
endif()

execute_process(
    COMMAND "${PDFLATEX_PATH}"
            -interaction=nonstopmode
            -halt-on-error
            -output-directory=${OUTPUT_DIR}
            "${_tex_path}"
    TIMEOUT 60
    RESULT_VARIABLE _latex_status
    OUTPUT_VARIABLE _latex_stdout
    ERROR_VARIABLE _latex_stderr
)
if(NOT _latex_status EQUAL 0)
    message(FATAL_ERROR
        "pdflatex failed with status ${_latex_status}:\n"
        "${_latex_stdout}${_latex_stderr}")
endif()

set(_log_path "${OUTPUT_DIR}/document.log")
if(NOT EXISTS "${_log_path}")
    message(FATAL_ERROR "pdflatex did not produce ${_log_path}")
endif()

file(READ "${_log_path}" _latex_log)
foreach(_unexpected "Overfull" "LaTeX Error" "Undefined control sequence")
    string(FIND "${_latex_log}" "${_unexpected}" _diagnostic_position)
    if(NOT _diagnostic_position EQUAL -1)
        message(FATAL_ERROR
            "Unexpected '${_unexpected}' in ${_log_path}")
    endif()
endforeach()
