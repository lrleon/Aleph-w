"""Conan recipe for Aleph-w (roadmap Phase 21).

Submission to conan-io/conan-center-index is a follow-up; this recipe builds
the library from the in-tree sources and exposes the documented options.
"""

import os

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import copy, load


class AlephWConan(ConanFile):
    name = "aleph-w"
    license = "MIT"
    url = "https://github.com/lrleon/Aleph-w"
    homepage = "https://github.com/lrleon/Aleph-w"
    description = (
        "Header-dominant C++20 library of data structures, algorithms and a "
        "cellular-automata framework."
    )
    topics = ("algorithms", "data-structures", "cellular-automata", "cpp20")
    settings = "os", "compiler", "build_type", "arch"

    options = {
        "use_x11": [True, False],
        # The FFmpeg sink (ca-ffmpeg-sink.H) shells out to the ffmpeg binary at
        # runtime and has no build-time dependency, so this option is reserved
        # for forward compatibility and currently does not change the build.
        "use_ffmpeg": [True, False],
        "build_examples": [True, False],
    }
    default_options = {
        "use_x11": False,
        "use_ffmpeg": False,
        "build_examples": False,
    }

    exports_sources = (
        "CMakeLists.txt", "VERSION.txt", "LICENSE", "cmake/*", "*.H", "*.h",
        "*.C", "*.cc", "*.c", "third_party/*", "Examples/*",
    )

    def set_version(self):
        self.version = load(self, os.path.join(self.recipe_folder, "VERSION.txt")).strip()

    def requirements(self):
        self.requires("gmp/6.3.0")
        self.requires("mpfr/4.2.1")
        self.requires("gsl/2.7.1")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["BUILD_TESTS"] = False
        tc.variables["BUILD_EXAMPLES"] = self.options.build_examples
        tc.variables["BUILD_REPRODUCTIONS"] = False
        tc.variables["BUILD_BENCHMARKS"] = False
        tc.variables["ALEPH_BUILD_X11_VIEWER"] = self.options.use_x11
        tc.generate()
        CMakeDeps(self).generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build(target="Aleph")

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, "LICENSE", src=self.source_folder,
             dst=os.path.join(self.package_folder, "licenses"))

    def package_info(self):
        # Match the CMake package config so find_package(Aleph) keeps working
        # for consumers that resolve the dependency through Conan.
        self.cpp_info.set_property("cmake_file_name", "Aleph")
        self.cpp_info.set_property("cmake_target_name", "Aleph::Aleph")
        self.cpp_info.libs = ["Aleph", "aleph_miniz"]
        self.cpp_info.includedirs = ["include/aleph"]
        if self.settings.os in ("Linux", "FreeBSD"):
            self.cpp_info.system_libs = ["pthread", "m"]
        if self.options.use_x11:
            self.cpp_info.system_libs.append("X11")