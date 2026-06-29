# Contributing to Aleph-w

Thanks for your interest in improving Aleph-w! This document describes how to
set up your environment, the coding conventions we enforce, and the pull-request
workflow.

By participating you agree to abide by our
[Code of Conduct](CODE_OF_CONDUCT.md).

## Table of Contents

- [Getting Started](#getting-started)
- [Build & Test](#build--test)
- [Coding Conventions](#coding-conventions)
- [Documentation](#documentation)
- [Commit Messages](#commit-messages)
- [Pull-Request Workflow](#pull-request-workflow)
- [Developer Certificate of Origin (DCO)](#developer-certificate-of-origin-dco)
- [Reporting Bugs & Security Issues](#reporting-bugs--security-issues)

## Getting Started

Aleph-w is a C++20, header-dominant library. All code lives in
`namespace Aleph`. Required system libraries:

```bash
sudo apt-get install -y libgmp-dev libmpfr-dev libgsl-dev libx11-dev \
  libtclap-dev libgtest-dev
```

GoogleTest is auto-fetched if not installed.

## Build & Test

We use CMake with Ninja:

```bash
# Configure and build
cmake -S . -B build -G Ninja
cmake --build build

# Build a single test target
cmake --build build --target geom_algorithms_test

# Run all tests
ctest --test-dir build --output-on-failure

# Run tests matching a pattern (CTest regex)
ctest --test-dir build -R dynlist --output-on-failure

# Run specific GTest cases within a binary
./build/Tests/geom_algorithms_test --gtest_filter="*ShortestPath*"
```

Tests live in `Tests/*.cc`; each `.cc` produces `build/Tests/<name>`.

> **Note:** Release builds **require** `-fno-strict-aliasing` due to
> `htlist.H`. The CMake configuration already propagates this; do not strip it.

Before opening a PR, make sure:

- `cmake --build build` is clean (no new warnings).
- `ctest --test-dir build --output-on-failure` is green.
- New public APIs are documented (see [Documentation](#documentation)).

## Coding Conventions

These are enforced in review; please follow them to keep diffs small.

- **Header naming**: `tpl_<name>.H` (templates), `ah-<name>.H` (utilities),
  `<Algorithm>.H` (algorithms). Headers are flat in the repository root.
- **Style**: 2-space indentation, GNU/K&R braces, traditional include guards
  (no `#pragma once`).
- **Operators**: prefer `and`, `or`, `not` over `&&`, `||`, `!`.
- **Containers**: **always** prefer Aleph containers (`Array`, `DynList`,
  `DynSetTree`, …) over the STL. Documented exceptions live in
  [`docs/std-vector-whitelist.md`](docs/std-vector-whitelist.md).
- **Error handling**: use the Aleph macros from `ah-errors.H`
  (`ah_out_of_range_error_if`, …). Never use raw `throw`.

## Documentation

Public APIs require Doxygen comments in **English**: `@brief`, `@param`,
`@return`, `@throws`, `@note`. Headers in the cellular-automata module also
carry `@par Complexity` and `@par Thread-safety`. A CI gate
(`Header docstring coverage gate`) checks that changed public declarations are
documented.

## Commit Messages

This repository uses [Conventional Commits](https://www.conventionalcommits.org/).
The PR **title** is linted (`conventional-pr` workflow) because we squash-merge,
so the PR title becomes the commit on `master`. Examples:

```
feat(ca): add per-axis boundary mix
fix(core): correct off-by-one in DynList::remove
docs(readme): document find_package usage
refactor(core): use get_it() for container iterators
```

Allowed types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`,
`build`, `ci`, `chore`, `revert`. Breaking changes use `!` (e.g. `feat(ca)!:`)
and must be described in [`CHANGELOG.md`](CHANGELOG.md).

## Pull-Request Workflow

1. Fork and create a topic branch off `master`
   (`git checkout -b feat/my-change`).
2. Make focused commits. Keep one logical change per PR; large phases should be
   split into several PRs (see `cell-automata-industrial-roadmap.md`).
3. Add or update tests for any behavioral change.
4. Update `CHANGELOG.md` under the appropriate section
   (Added/Changed/Deprecated/Removed/Fixed/Security).
5. Ensure CI is green (build, tests, sanitizers, coverage, conventional-pr).
6. Open the PR with a Conventional-Commit-style title and a clear description.

We do not revert third-party or unrelated changes without explicit agreement.

## Developer Certificate of Origin (DCO)

We require the [Developer Certificate of Origin](https://developercertificate.org/).
Sign off every commit to certify you wrote the code or have the right to submit
it under the project's MIT license:

```bash
git commit -s -m "feat(ca): add per-axis boundary mix"
```

This appends a `Signed-off-by: Your Name <you@example.com>` trailer. Commits
without a sign-off may be asked to amend before merge.

## Reporting Bugs & Security Issues

- **Bugs / feature requests**: open a GitHub issue with a minimal reproducer.
- **Security vulnerabilities**: **do not** open a public issue — follow
  [`SECURITY.md`](SECURITY.md) for private disclosure.