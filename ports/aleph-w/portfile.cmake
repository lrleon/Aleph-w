# vcpkg port for Aleph-w (roadmap Phase 21).
#
# Submission to microsoft/vcpkg is a follow-up: the SHA512 below must be the
# real hash of the v${VERSION} release tarball. During local development run
# `vcpkg install aleph-w --overlay-ports=ports` once with SHA512 set to all
# zeros; vcpkg prints the expected hash, which is then pasted here.

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO lrleon/Aleph-w
    REF "v${VERSION}"
    SHA512 0
    HEAD_REF master
)

# X11 is opt-in via the `x11` feature; the library otherwise compiles its
# viewer to a no-op stub and drops the libX11 dependency.
set(ALEPH_X11 OFF)
if("x11" IN_LIST FEATURES)
    set(ALEPH_X11 ON)
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_EXAMPLES=OFF
        -DBUILD_REPRODUCTIONS=OFF
        -DBUILD_BENCHMARKS=OFF
        -DALEPH_BUILD_X11_VIEWER=${ALEPH_X11}
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Aleph CONFIG_PATH lib/cmake/Aleph)

# A static library ships no debug headers of its own.
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")