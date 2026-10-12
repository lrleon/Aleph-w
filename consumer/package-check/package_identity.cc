/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file package_identity.cc
 * @brief Installed-package check: aleph-version.H says what find_package says.
 *
 * CMake passes what `find_package(Aleph)` reported (`EXPECT_*`); the header
 * of the same installation must report the same release number, source
 * revision and sanitizer configuration.
 */

#include <cstdlib>
#include <cstring>
#include <iostream>

#include <aleph-version.H>

static_assert(ALEPH_VERSION_MAJOR == EXPECT_VERSION_MAJOR);
static_assert(ALEPH_VERSION_MINOR == EXPECT_VERSION_MINOR);
static_assert(ALEPH_VERSION_PATCH == EXPECT_VERSION_PATCH);
static_assert(ALEPH_VERSION_NUMBER
              == EXPECT_VERSION_MAJOR * 1000000 + EXPECT_VERSION_MINOR * 1000 + EXPECT_VERSION_PATCH);
static_assert(ALEPH_BUILT_WITH_SANITIZERS == EXPECT_SANITIZERS);
static_assert(ALEPH_BUILT_WITH_CONTAINER_ANNOTATIONS == EXPECT_CONTAINER_ANNOTATIONS);

static_assert(Aleph::Build_Info::version_major == ALEPH_VERSION_MAJOR);
static_assert(Aleph::Build_Info::version_minor == ALEPH_VERSION_MINOR);
static_assert(Aleph::Build_Info::version_patch == ALEPH_VERSION_PATCH);
static_assert(Aleph::Build_Info::sanitizers == (EXPECT_SANITIZERS != 0));
static_assert(Aleph::Build_Info::container_annotations == (EXPECT_CONTAINER_ANNOTATIONS != 0));

// The usage requirements of Aleph::Aleph carry the configuration of the
// package into this translation unit.
#if EXPECT_CONTAINER_ANNOTATIONS
# ifndef ALEPH_ANNOTATE_CONTAINERS
#  error "Aleph::Aleph did not define ALEPH_ANNOTATE_CONTAINERS"
# endif
#endif

int main()
{
  int failures = 0;

  if (std::strcmp(Aleph::Build_Info::version, EXPECT_VERSION) != 0)
    {
      std::cerr << "release number: the header says " << Aleph::Build_Info::version
                << ", the package says " << EXPECT_VERSION << '\n';
      ++failures;
    }

  if (std::strcmp(Aleph::Build_Info::source_revision, EXPECT_SOURCE_REVISION) != 0)
    {
      std::cerr << "source revision: the header says " << Aleph::Build_Info::source_revision
                << ", the package says " << EXPECT_SOURCE_REVISION << '\n';
      ++failures;
    }

  if (std::strcmp(Aleph::Build_Info::source_revision, ALEPH_SOURCE_REVISION) != 0
      or std::strlen(Aleph::Build_Info::source_revision) == 0)
    {
      std::cerr << "source revision: empty or different from ALEPH_SOURCE_REVISION\n";
      ++failures;
    }

  std::cout << "Aleph-w " << Aleph::Build_Info::version
            << ", revision " << Aleph::Build_Info::source_revision
            << ", sanitizers " << Aleph::Build_Info::sanitizers
            << ", container annotations " << Aleph::Build_Info::container_annotations << '\n';

  return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
