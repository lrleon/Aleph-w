# Copyright Spack Project Developers. See the Spack LICENSE file for details.
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

# Spack package for Aleph-w (roadmap Phase 21, optional HPC-community target).
# Upstreaming to spack/spack is a follow-up: the version sha256 must be the
# hash of the corresponding release tarball, obtained with `spack checksum`.

from spack.package import *


class AlephW(CMakePackage):
    """Header-dominant C++20 library of data structures, algorithms and a
    cellular-automata framework."""

    homepage = "https://github.com/lrleon/Aleph-w"
    url = "https://github.com/lrleon/Aleph-w/archive/refs/tags/v4.0.0.tar.gz"
    git = "https://github.com/lrleon/Aleph-w.git"

    maintainers("lrleon")

    license("MIT")

    version("master", branch="master")
    version("4.0.0", sha256="0000000000000000000000000000000000000000000000000000000000000000")

    variant("x11", default=False, description="Build the X11 live viewer")

    depends_on("cxx", type="build")
    depends_on("cmake@3.18:", type="build")
    depends_on("gmp")
    depends_on("mpfr")
    depends_on("gsl")
    depends_on("libx11", when="+x11")

    def cmake_args(self):
        return [
            self.define("BUILD_TESTS", False),
            self.define("BUILD_EXAMPLES", False),
            self.define("BUILD_REPRODUCTIONS", False),
            self.define("BUILD_BENCHMARKS", False),
            self.define_from_variant("ALEPH_BUILD_X11_VIEWER", "x11"),
        ]