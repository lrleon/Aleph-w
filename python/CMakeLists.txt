# --------------------------------------------------------------------------
# Aleph-w — Python bindings (Phase 18)
# --------------------------------------------------------------------------
#
# Builds the `aleph_ca._core` pybind11 extension and packages the Python
# façade located in `python/aleph_ca/`. The module is opt-in: top-level
# CMakeLists wires it behind the `ALEPH_BUILD_PYTHON` option (default OFF).
#
# When invoked through scikit-build-core (see python/pyproject.toml) this
# subdirectory is the install target; the generated extension lands inside
# the wheel under `aleph_ca/_core.<abi>.so`.
#
# Dependencies:
#   - Python3 with development headers
#   - pybind11 >= 2.11 (auto-fetched if not found)
#   - The parent `Aleph` static library (already declared at top level)
# --------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.18)

# When configured as a stand-alone build (scikit-build-core invokes this
# file directly), promote the embedding directory into a full project so
# `find_package(Python ...)` and other top-level helpers behave normally.
if(NOT DEFINED ALEPH_PYTHON_BINDINGS_INTERNAL)
    message(STATUS "Aleph-w Python bindings: stand-alone configuration")
    project(aleph_ca_bindings LANGUAGES CXX)
endif()

# ---- Python ---------------------------------------------------------------
find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)

# ---- pybind11 -------------------------------------------------------------
# Prefer a system/installed pybind11 (typical when invoked from cibuildwheel
# inside a venv that already pulled the build dependency). Fall back to
# FetchContent so the bindings remain buildable from a clean tree.
find_package(pybind11 2.11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# ---- Module target --------------------------------------------------------
pybind11_add_module(_core MODULE src/bindings.cc)

target_include_directories(_core PRIVATE
    ${CMAKE_SOURCE_DIR}
)

# The bindings instantiate engines whose template parameters are baked
# into the translation unit; we still need the Aleph runtime for the few
# .C files (ahDefs, ahAssert, ca-checkpoint-compress, ...).
if(TARGET Aleph)
    target_link_libraries(_core PRIVATE Aleph)
else()
    message(FATAL_ERROR
        "aleph_ca._core requires the Aleph target. Configure the top-level "
        "CMakeLists with ALEPH_BUILD_PYTHON=ON or invoke scikit-build-core "
        "via python/pyproject.toml.")
endif()

# Match the rest of the codebase: avoid strict aliasing (htlist.H caveat
# documented at top-level) and keep symbol visibility tight on the .so.
target_compile_features(_core PRIVATE cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(_core PRIVATE
        -fno-strict-aliasing
        -fvisibility=hidden
    )
endif()

# ---- Installation ---------------------------------------------------------
# scikit-build-core honours the `aleph_ca` package layout: install the
# compiled extension next to the Python façade so `import aleph_ca` keeps
# working both from the wheel and from an in-tree development build.
if(DEFINED SKBUILD_PROJECT_NAME)
    install(TARGETS _core LIBRARY DESTINATION aleph_ca)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/aleph_ca/
            DESTINATION aleph_ca
            FILES_MATCHING PATTERN "*.py")
else()
    # In-tree CMake builds: place the .so under build/python/aleph_ca/ so
    # developers can `PYTHONPATH=build/python python -c 'import aleph_ca'`.
    set_target_properties(_core PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python/aleph_ca"
    )
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/aleph_ca/
         DESTINATION ${CMAKE_BINARY_DIR}/python/aleph_ca
         FILES_MATCHING PATTERN "*.py")
endif()

message(STATUS "Aleph-w Python bindings: target _core configured")