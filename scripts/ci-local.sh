#!/usr/bin/env bash
#
# Runs on this machine, in Docker containers, the Linux jobs of
# .github/workflows/ci.yml that decide most pull requests, before pushing:
# the required checks of the ruleset protect-master (the eight
# build-and-test jobs and sanitizers), the other sanitizers, the graph
# harness and the policy gates. Each job runs in an image of the Ubuntu of
# its job in the CI, with the packages, the compilers and the CMake options
# of that job, on the working tree or on a commit, with more cores than a
# runner. macOS, Windows and Linux on arm64 stay in the CI.
#
# Usage: scripts/ci-local.sh [options] [job|group]...
#
#   Groups: required (the default), all-sanitizers, harness, all.
#   Jobs:   scripts/ci-local.sh --list
#
# Options:
#   -j, --jobs N   jobs at a time (default 3)
#   --cpus N       cores for all the jobs (default: those of the machine
#                  but 2); each job builds and tests with cpus / jobs
#   --rev REV      the tree of the commit REV instead of the working tree
#   --merge        the merge of the commit REV (default HEAD) into
#                  origin/master, which the CI tests for a pull request,
#                  after fetching origin master
#   --publish      with --merge, when the required jobs pass, post on the
#                  commit in GitHub the status ci-local/required, which
#                  lets the CI of a pull request with the label ci-local
#                  skip them (see docs/ci-local.md); the status is pending
#                  while the jobs run, and a failure if one fails; a
#                  result that passed before for the same commit, master,
#                  workflow and week is posted without running the jobs
#                  again
#   --dry-run      with --publish, print the status instead of posting it
#   --clean        start the build directories of the jobs from scratch
#   --list         list the jobs and the groups
#   --show JOB     print the commands that JOB runs in its container
#   -h, --help     this help
#
# The working tree is copied to $ALEPH_CI_LOCAL_DIR/src (default
# ~/.cache/aleph-ci-local): the tracked files as they are, and the
# untracked ones that are not ignored inside the directories that the
# repository tracks, or with the extension of a source at its root. The
# containers mount that copy read-only; their build directories and ccache
# stay there between runs, so that a run after a change rebuilds what the
# change touches. The images (aleph-ci-local:<release>-<hash>) are built
# from the Dockerfile below once a week, from the latest base image and
# packages, as the CI installs the latest ones in every run; the first run
# of a week builds them and compiles from scratch. The policy job runs on
# this machine (git, ruby, doxygen and the remote of the repository), on a
# temporary clone where the working tree is a commit.
#
# Exit status: 0 if every job passes. The log of each job, and a summary,
# go to $ALEPH_CI_LOCAL_DIR/logs/<date>/ (latest: logs/latest).
#
# On a machine of 20 cores, the first run of required, which builds the
# two images and every job from scratch, took 23 minutes, and a run after
# it, 5 minutes (the CI takes 17); the sixteen jobs keep some 35 GB of
# build directories and ccache. To free them: rm -rf "$ALEPH_CI_LOCAL_DIR".

set -euo pipefail

SCRIPT_DIR="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
SELF="${SCRIPT_DIR}/$(basename "${BASH_SOURCE[0]}")"
ROOT_DIR="$(cd -P "${SCRIPT_DIR}/.." && pwd -P)"
WORK="${ALEPH_CI_LOCAL_DIR:-${HOME}/.cache/aleph-ci-local}"
TAG="ci-local"

JOBS=3
CPUS=$(( $(nproc) > 4 ? $(nproc) - 2 : $(nproc) ))
REV=""
MERGE=0
PUBLISH=0
DRY_RUN=0
CLEAN=0
SHOW=""
REQUESTED=()
# The status that --publish posts, which the job local-ci of the CI looks
# for; and what --merge tests: the commit, the master it merges into, the
# tree of the merge and the fingerprint of the workflow and this script
STATUS_CONTEXT="ci-local/required"
HEAD_SHA=""
BASE=""
TREE=""
FP=""
WEEK="$(date +%G-W%V)"

say() { echo "[${TAG}] $*"; }
die() { echo "[${TAG}] $*" >&2; exit 1; }

# The jobs: kind, Ubuntu release, compiler, build type, and the minutes
# that they took in the CI, which order them, the longest first
declare -A KIND REL COMP BTYPE MINUTES
add_job() { KIND[$1]=$2; REL[$1]=$3; COMP[$1]=$4; BTYPE[$1]=$5; MINUTES[$1]=$6; }
for rel in 22.04 24.04; do
  for comp in gcc clang; do
    for bt in Debug Release; do
      add_job "bt-${rel}-${comp}-${bt,,}" bt "${rel}" "${comp}" "${bt}" 5
    done
  done
done
MINUTES[bt-24.04-clang-debug]=7; MINUTES[bt-22.04-gcc-debug]=6; MINUTES[bt-24.04-gcc-release]=6
add_job sanitizers sanitizers 24.04 gcc Debug 17
add_job tsan tsan 24.04 clang Debug 15
add_job ubsan ubsan 24.04 clang Debug 7
add_job harness-24.04-gcc-release harness 24.04 gcc Release 2
add_job harness-22.04-clang-release harness 22.04 clang Release 3
add_job harness-asan-exhaustive harness 24.04 gcc Debug 10
add_job harness-asan-contract harness 24.04 gcc Debug 2
add_job policy policy host - - 9

# The groups, which go through ${!name}, that shellcheck does not follow
# shellcheck disable=SC2034
{
GROUP_required="bt-22.04-gcc-debug bt-22.04-gcc-release bt-22.04-clang-debug \
bt-22.04-clang-release bt-24.04-gcc-debug bt-24.04-gcc-release bt-24.04-clang-debug \
bt-24.04-clang-release sanitizers"
GROUP_all_sanitizers="sanitizers tsan ubsan"
GROUP_harness="harness-24.04-gcc-release harness-22.04-clang-release \
harness-asan-exhaustive harness-asan-contract"
GROUP_all="${GROUP_required} tsan ubsan ${GROUP_harness} policy"
}
GROUP_NAMES="required all-sanitizers harness all"

describe() {  # job -> the name of its job in the CI
  local j=$1
  case "${KIND[$j]}" in
    bt) echo "build-and-test (ubuntu-${REL[$j]}, ${COMP[$j]}, ${BTYPE[$j]})";;
    sanitizers) echo "sanitizers (ubuntu-24.04, gcc, Debug)";;
    tsan) echo "tsan (ubuntu-24.04, clang, Debug)";;
    ubsan) echo "ubsan (ubuntu-24.04, clang, Debug)";;
    harness)
      case "$j" in
        harness-asan-exhaustive) echo "graph harness (ubuntu-24.04, gcc, Debug, ASan and UBSan, exhaustive)";;
        harness-asan-contract) echo "graph harness (ubuntu-24.04, gcc, Debug, ASan and UBSan, contract_matrix)";;
        *) echo "graph harness (ubuntu-${REL[$j]}, ${COMP[$j]}, ${BTYPE[$j]})";;
      esac;;
    policy) echo "policy-checks";;
  esac
}

list_jobs() {
  local j
  echo "Jobs (as in .github/workflows/ci.yml):"
  for j in ${GROUP_all}; do printf '  %-28s %s\n' "$j" "$(describe "$j")"; done
  echo "Groups:"
  for g in ${GROUP_NAMES}; do
    local v="GROUP_${g//-/_}"
    printf '  %-15s %s\n' "$g" "$(echo ${!v})"
  done
}

usage() { sed -n '3,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

while [ $# -gt 0 ]; do
  case "$1" in
    -j|--jobs) JOBS=${2:?}; shift 2;;
    --cpus) CPUS=${2:?}; shift 2;;
    --rev) REV=${2:?}; shift 2;;
    --merge) MERGE=1; shift;;
    --publish) PUBLISH=1; shift;;
    --dry-run) DRY_RUN=1; shift;;
    --clean) CLEAN=1; shift;;
    --list) list_jobs; exit 0;;
    --show) SHOW=${2:?}; shift 2;;
    -h|--help) usage; exit 0;;
    -*) die "unknown option: $1 (see --help)";;
    *) REQUESTED+=("$1"); shift;;
  esac
done
[ ${#REQUESTED[@]} -eq 0 ] && REQUESTED=(required)

# The jobs that were asked, without repetitions, the longest first
SELECTED=()
for x in "${REQUESTED[@]}"; do
  if [[ " ${GROUP_NAMES} " == *" ${x} "* ]]; then
    v="GROUP_${x//-/_}"
    for j in ${!v}; do SELECTED+=("$j"); done
  elif [ -n "${KIND[$x]:-}" ]; then
    SELECTED+=("$x")
  else
    die "unknown job or group: $x (see --list)"
  fi
done
mapfile -t SELECTED < <(for j in "${SELECTED[@]}"; do echo "${MINUTES[$j]} $j"; done |
                        sort -u -k2,2 | sort -k1,1nr | awk '{print $2}')

if (( PUBLISH )); then
  (( MERGE )) || die "--publish needs --merge: the CI tests the merge into master"
  for j in ${GROUP_required}; do
    [[ " ${SELECTED[*]} " == *" ${j} "* ]] || die "--publish needs every job of the group required"
  done
  (( DRY_RUN )) || command -v gh >/dev/null || die "gh is missing: --publish posts the status with it"
fi
(( DRY_RUN && ! PUBLISH )) && die "--dry-run goes with --publish"
(( MERGE )) && REV="${REV:-HEAD}"

[[ "${JOBS}" =~ ^[1-9][0-9]*$ ]] || die "--jobs takes a positive number"
[[ "${CPUS}" =~ ^[1-9][0-9]*$ ]] || die "--cpus takes a positive number"
(( JOBS > ${#SELECTED[@]} )) && JOBS=${#SELECTED[@]}
NCPU=$(( CPUS / JOBS )); (( NCPU < 2 )) && NCPU=2
# A build under the sanitizers takes up to some 2 GB by core: no more
# cores by job than the available memory gives, where Linux tells it
mem_gb=$(awk '/^MemAvailable:/ {print int($2 / 1048576)}' \
         /proc/meminfo 2>/dev/null || true)
if [[ "${mem_gb}" =~ ^[1-9][0-9]*$ ]]; then
  max_ncpu=$(( mem_gb / (JOBS * 2) ))
  (( max_ncpu < 2 )) && max_ncpu=2
  if (( NCPU > max_ncpu )); then NCPU=${max_ncpu}; fi
fi

command -v git >/dev/null || die "git is missing"
needs_docker=0
for j in "${SELECTED[@]}"; do [ "${KIND[$j]}" != policy ] && needs_docker=1; done
if (( needs_docker )); then
  command -v docker >/dev/null || die "docker is missing"
  docker info >/dev/null 2>&1 || die "cannot use docker (is this user in the group docker?)"
fi
# The runners of the CI are x86_64: on another machine Docker takes the
# images and the compilers of its architecture, which the status of
# --publish would not certify
if (( PUBLISH )); then
  arch=$(docker info --format '{{.Architecture}}' 2>/dev/null || true)
  case "${arch}" in
    x86_64|amd64) ;;
    *) if (( DRY_RUN )); then
         say "warning: Docker runs on ${arch:-an unknown architecture}, not x86_64: --publish would refuse"
       else
         die "--publish needs Docker on x86_64, as the runners of the CI (this one: ${arch:-unknown})"
       fi;;
  esac
fi
command -v rsync >/dev/null || die "rsync is missing"

# The Dockerfile of the images: the packages of the jobs in the CI, with
# their recommended ones, as apt-get there (the runtimes of the sanitizers
# of Clang come so), and GoogleTest built with libc++ for the Clang jobs, as
# they build it
dockerfile() {  # release
  cat <<EOF
FROM ubuntu:$1
ENV DEBIAN_FRONTEND=noninteractive LANG=C.UTF-8
RUN apt-get update && apt-get install -y \\
      cmake ninja-build pkg-config ccache make file git ca-certificates \\
      g++ clang libc++-dev libc++abi-dev \\
      libgmp-dev libmpfr-dev libgsl-dev libx11-dev libxext-dev \\
      libpthread-stubs0-dev libtclap-dev libgtest-dev ruby-full python3 \\
 && rm -rf /var/lib/apt/lists/*
RUN cmake -S /usr/src/googletest -B /tmp/gtest -G Ninja \\
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \\
      -DCMAKE_CXX_FLAGS=-stdlib=libc++ -DCMAKE_EXE_LINKER_FLAGS=-lc++abi \\
      -DCMAKE_INSTALL_PREFIX=/opt/gtest-libcxx \\
 && cmake --build /tmp/gtest && cmake --install /tmp/gtest && rm -rf /tmp/gtest
EOF
}

declare -A IMAGE TOOLS
ensure_image() {  # release
  local rel=$1 hash tag ctx
  # the week goes into the hash, so that a new week builds the image anew,
  # from the latest base image and packages
  hash=$( { dockerfile "$rel"; echo "week ${WEEK}"; } | sha256sum | cut -c1-12)
  tag="aleph-ci-local:${rel}-${hash}"
  IMAGE[$rel]=$tag
  if ! docker image inspect "$tag" >/dev/null 2>&1; then
    say "building the image ${tag} (once a week)"
    ctx=$(mktemp -d)
    dockerfile "$rel" > "${ctx}/Dockerfile"
    # without the cache of the layers, which would keep the packages of the
    # image before: --pull refreshes only the base image
    docker build --pull --no-cache -q -t "$tag" "$ctx" > "${LOGS}/image-${rel}.log" 2>&1 ||
      { rm -rf "$ctx"; die "the image ${tag} did not build: ${LOGS}/image-${rel}.log"; }
    rm -rf "$ctx"
    # the images of the release of a Dockerfile, or of a week, before this one
    docker image ls --format '{{.Repository}}:{{.Tag}}' aleph-ci-local |
      grep -E "^aleph-ci-local:${rel//./\\.}-" | grep -vx "${tag}" | xargs -r docker rmi >/dev/null 2>&1 || true
  fi
  # the versions of the compilers and of CMake in it, for the summary and
  # the status that --publish posts
  TOOLS[$rel]=$(docker run --rm "$tag" bash -c \
    'echo "gcc $(gcc -dumpfullversion) clang $(clang -dumpversion) cmake $(cmake --version | awk "NR == 1 {print \$3}")"' 2>/dev/null) ||
    TOOLS[$rel]="unknown"
}

# The untracked files that are not ignored and that a commit would take:
# those inside a directory that the repository tracks, and those with the
# extension of a source at its root
untracked_sources() {
  local dirs
  dirs=$(git -C "${ROOT_DIR}" ls-tree -d --name-only HEAD | paste -sd'|')
  git -C "${ROOT_DIR}" ls-files -z -o --exclude-standard |
    grep -zE "^((${dirs})/|[^/]+\.(H|h|hpp|hh|hxx|C|cc|cpp|cxx|c)$)" || true
}

SRC=""
BUILD_ROOT=""
prepare_source() {
  if [ -n "${REV}" ]; then
    local sha what
    sha=$(git -C "${ROOT_DIR}" rev-parse --verify "${REV}^{commit}") || die "no commit ${REV}"
    what=${sha}
    if (( MERGE )); then
      # what the CI tests for a pull request: the merge of the commit into
      # the master of GitHub, which must have no conflicts
      git -C "${ROOT_DIR}" fetch -q origin master || die "cannot fetch origin master"
      BASE=$(git -C "${ROOT_DIR}" rev-parse --verify "origin/master^{commit}")
      local out rc=0
      out=$(git -C "${ROOT_DIR}" merge-tree --write-tree "${BASE}" "${sha}" 2>&1) || rc=$?
      (( rc == 1 )) && die "${sha:0:12} does not merge into origin/master ${BASE:0:12} without conflicts"
      (( rc == 0 )) || die "git merge-tree --write-tree failed (it needs git 2.38 or later): ${out%%$'\n'*}"
      TREE=${out%%$'\n'*}
      HEAD_SHA=${sha}
      what=${TREE}
      # the fingerprint that the job local-ci of the CI computes on the
      # merge: the bytes of the workflow and of this script
      FP=$( { git -C "${ROOT_DIR}" show "${TREE}:.github/workflows/ci.yml"
              git -C "${ROOT_DIR}" show "${TREE}:scripts/ci-local.sh"; } | sha256sum | cut -c1-12)
      # the jobs that run are those of this script, and the fingerprint
      # names those of the merge: they must be the same script
      if ! git -C "${ROOT_DIR}" show "${TREE}:scripts/ci-local.sh" | cmp -s - "${SELF}"; then
        die "this script is not the scripts/ci-local.sh of the merge into origin/master ${BASE:0:12}: commit its changes, or bring master into the branch if master changed it"
      fi
    fi
    SRC="${WORK}/src-rev"
    rm -rf "${SRC}"; mkdir -p "${SRC}"
    git -C "${ROOT_DIR}" archive "${what}" | tar -x -C "${SRC}"
    # git archive gives the files the date of the commit, or the current one
    # for a tree: an object newer than a changed source would look up to
    # date, so these build anew
    BUILD_ROOT="${WORK}/build-rev"
    rm -rf "${BUILD_ROOT}"
    if (( MERGE )); then
      say "source: the merge of ${sha:0:12} ($(git -C "${ROOT_DIR}" log -1 --format=%s "${sha}")) into origin/master ${BASE:0:12}"
    else
      say "source: commit ${sha:0:12} ($(git -C "${ROOT_DIR}" log -1 --format=%s "${sha}"))"
    fi
  else
    SRC="${WORK}/src"
    BUILD_ROOT="${WORK}/build"
    mkdir -p "${SRC}"
    local list="${WORK}/src.list"
    # a tracked file deleted from the working tree stays out; an if, so that
    # it does not leave the status of the loop, and of the pipe, at 1
    { git -C "${ROOT_DIR}" ls-files -z -c; untracked_sources; } |
      while IFS= read -r -d '' f; do
        if [ -e "${ROOT_DIR}/$f" ] || [ -L "${ROOT_DIR}/$f" ]; then
          printf '%s\0' "$f"
        fi
      done | sort -zu > "${list}"
    rsync -a --checksum --from0 --files-from="${list}" "${ROOT_DIR}/" "${SRC}/"
    # what the working tree no longer has goes from the copy too
    comm -z -23 <(cd "${SRC}" && find . \( -type f -o -type l \) -printf '%P\0' | sort -z) \
                "${list}" | (cd "${SRC}" && xargs -0 -r rm -f)
    local changed
    changed=$(git -C "${ROOT_DIR}" status --porcelain --untracked-files=no | wc -l)
    say "source: working tree at $(git -C "${ROOT_DIR}" rev-parse --short HEAD), ${changed} changed files, $(tr -cd '\0' < "${list}" | wc -c) files"
  fi
  mkdir -p "${BUILD_ROOT}"
}

# The commands of a job in its container: those of its job in the CI, with
# NCPU cores instead of the 2 of a runner
job_script() {  # job
  local j=$1 kind=${KIND[$1]} rel=${REL[$1]} comp=${COMP[$1]} bt=${BTYPE[$1]}
  cat <<EOF
set -euo pipefail
cd /build
N=${NCPU}
if [ "${comp}" = clang ]; then
  export CC=clang CXX=clang++
  STDLIB=-stdlib=libc++ LDFLAGS_X=-lc++abi
  GTEST="-DGTest_DIR=/opt/gtest-libcxx/lib/cmake/GTest"
else
  export CC=gcc CXX=g++
  STDLIB= LDFLAGS_X= GTEST=
fi
COMMON="-S /src -B /build -G Ninja -DCMAKE_C_COMPILER_LAUNCHER=ccache \
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=${bt} -DBUILD_EXAMPLES=OFF \
-DBUILD_TESTS=ON"
golden() { mkdir -p /build/test_artifacts/golden_svg; export ALEPH_TEST_GOLDEN_DIR=/build/test_artifacts/golden_svg; }
echo "== \$(${comp/gcc/g++} --version | head -1); \$(cmake --version | head -1); \${N} cores"
EOF
  case "${kind}" in
    bt)
      local checks=OFF
      [ "${rel}" = 24.04 ] && [ "${bt}" = Release ] && checks=ON
      cat <<EOF
cmake \${COMMON} -DCMAKE_CXX_FLAGS="\${STDLIB}" -DCMAKE_EXE_LINKER_FLAGS="\${LDFLAGS_X}" \
  -DALEPH_COMPILE_CHECKS=${checks} \${GTEST}
cmake --build /build --parallel \${N}
golden
ctest --test-dir /build --output-on-failure --parallel \${N} --timeout 600
EOF
      ;;
    sanitizers)
      cat <<EOF
cmake \${COMMON} -DALEPH_USE_SANITIZERS=ON
cmake --build /build --parallel \${N}
golden
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /build --output-on-failure --parallel \${N} --timeout 600
EOF
      ;;
    tsan|ubsan)
      local f opts
      if [ "${kind}" = tsan ]; then
        f="-fsanitize=thread"; opts="TSAN_OPTIONS=halt_on_error=1"
        cat <<EOF
cmake \${COMMON} -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="${f} -fno-omit-frame-pointer" -DCMAKE_CXX_FLAGS="${f} -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="${f}" -DCMAKE_SHARED_LINKER_FLAGS="${f}"
EOF
      else
        f="-fsanitize=undefined"; opts="UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1"
        cat <<EOF
cmake \${COMMON} -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_FLAGS="${f} -fno-sanitize-recover=undefined -fno-omit-frame-pointer" \
  -DCMAKE_CXX_FLAGS="${f} -fno-sanitize-recover=undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS="${f}" -DCMAKE_SHARED_LINKER_FLAGS="${f}"
EOF
      fi
      cat <<EOF
cmake --build /build --parallel \${N}
golden
${opts} ctest --test-dir /build --output-on-failure --parallel \${N} --timeout 600
EOF
      ;;
    harness)
      local san=OFF mode=normal suites="exhaustive contract_matrix" cn='${N}'
      case "$j" in
        harness-asan-exhaustive) san=ON; mode=light; suites=exhaustive;;
        harness-asan-contract) san=ON; mode=light; suites=contract_matrix;;
      esac
      # a test of the harness takes up to 5.4 GB to compile under the
      # sanitizers: at most 4 at a time
      [ "${san}" = ON ] && cn='$(( N < 4 ? N : 4 ))'
      cat <<EOF
cmake \${COMMON} -DCMAKE_CXX_FLAGS="\${STDLIB}" -DCMAKE_EXE_LINKER_FLAGS="\${LDFLAGS_X}" \
  -DALEPH_USE_SANITIZERS=${san} -DALEPH_GRAPH_HARNESS_TESTS=ON \${GTEST}
targets=\$(for s in ${suites}; do ls /src/Tests/graph_\${s}_*_test.cc; done | xargs -n1 basename | sed 's/\.cc\$//')
cmake --build /build --parallel ${cn} --target \${targets}
mkdir -p /build/harness-logs
run_one() { "\$1" --gtest_brief=1 > "/build/harness-logs/\$(basename "\$1").log" 2>&1 && echo "OK   \$(basename "\$1")" || echo "FAIL \$(basename "\$1")"; }
export -f run_one
export ALEPH_GRAPH_HARNESS=${mode} ASAN_OPTIONS=detect_leaks=1
for s in ${suites}; do ls /build/Tests/graph_\${s}_*_test; done | xargs -P "\${N}" -I{} bash -c 'run_one {}' | sort | tee /build/harness-logs/status
for f in \$(awk '\$1 == "FAIL" {print \$2}' /build/harness-logs/status); do echo "== \$f"; cat "/build/harness-logs/\$f.log"; done
! grep -q '^FAIL' /build/harness-logs/status
EOF
      ;;
  esac
}

# The policy gates of the CI on a temporary clone of this repository where
# the working tree (or the commit) is the commit of a pull request against
# master, with the remote of the repository
policy_job() {
  local clone="${WORK}/policy-clone" sha
  for c in ruby doxygen; do
    command -v "$c" >/dev/null || { echo "$c is missing: the policy job needs ruby and doxygen"; return 1; }
  done
  rm -rf "${clone}"
  git clone -q --shared --no-checkout "${ROOT_DIR}" "${clone}" || return 1
  if (( MERGE )); then
    # the merge that the other jobs test, as a commit of the two parents,
    # as the CI checks it out
    sha=$(git -C "${clone}" -c user.name="${TAG}" -c user.email="${TAG}@localhost" \
            commit-tree "${TREE}" -p "${BASE}" -p "${HEAD_SHA}" -m "${TAG}: the merge") || return 1
    git -C "${clone}" checkout -q --detach "${sha}" || return 1
    echo "policy on the merge of ${HEAD_SHA:0:12} into ${BASE:0:12}"
  elif [ -n "${REV}" ]; then
    sha=$(git -C "${ROOT_DIR}" rev-parse --verify "${REV}^{commit}")
    git -C "${clone}" checkout -q --detach "${sha}" || return 1
  else
    git -C "${clone}" checkout -q --detach "$(git -C "${ROOT_DIR}" rev-parse HEAD)" || return 1
    local patch="${WORK}/policy.patch"
    git -C "${ROOT_DIR}" diff --binary HEAD > "${patch}"
    if [ -s "${patch}" ]; then
      git -C "${clone}" apply --index "${patch}" || { echo "the changes of the working tree did not apply"; return 1; }
    fi
    while IFS= read -r -d '' f; do
      mkdir -p "${clone}/$(dirname "$f")" && cp -p "${ROOT_DIR}/$f" "${clone}/$f" &&
        git -C "${clone}" add -- "$f" || { echo "could not add $f"; return 1; }
    done < <(untracked_sources)
    git -C "${clone}" -c user.name="${TAG}" -c user.email="${TAG}@localhost" \
      commit -q --allow-empty -m "${TAG}: the working tree" || return 1
  fi
  git -C "${clone}" remote set-url origin "$(git -C "${ROOT_DIR}" remote get-url origin)" || return 1
  (
    cd "${clone}"
    export GITHUB_EVENT_NAME=pull_request GITHUB_BASE_REF=master
    status=0
    ruby scripts/ci_policy_checks.rb || status=1
    ruby scripts/ci_doxygen_changed.rb || status=1
    ruby scripts/test_ci_header_doc_coverage.rb || status=1
    ruby scripts/ci_header_doc_coverage.rb --min 80 || status=1
    exit ${status}
  )
}

run_job() {  # job; writes its status and its time to the logs
  local j=$1 log="${LOGS}/$1.log" t0 rc=0
  t0=$(date +%s)
  {
    echo "== $(describe "$j")"
    if [ "${KIND[$j]}" = policy ]; then
      policy_job || rc=$?
    else
      local rel=${REL[$j]} name="aleph-ci-local-${j}-$$"
      local build="${BUILD_ROOT}/${j}" cache="${WORK}/ccache/${rel}"
      (( CLEAN )) && rm -rf "${build}"
      mkdir -p "${build}" "${cache}"
      docker run --rm --init --name "${name}" \
        -u "$(id -u):$(id -g)" -e HOME=/tmp -e CI=true -e LANG=C.UTF-8 \
        -e CCACHE_DIR=/ccache -e CCACHE_MAXSIZE=30G \
        -v "${SRC}:/src:ro" -v "${build}:/build" -v "${cache}:/ccache" \
        --cpus "${NCPU}" --security-opt seccomp=unconfined --cap-add SYS_PTRACE \
        "${IMAGE[$rel]}" bash -c "$(job_script "$j")" || rc=$?
    fi
  } > "${log}" 2>&1
  echo "${rc} $(( $(date +%s) - t0 ))" > "${LOGS}/$j.status"
  say "$( (( rc == 0 )) && echo "PASS" || echo "FAIL") $j ($(( ($(date +%s) - t0) / 60 )) min)"
}

# Where --publish posts: the repository in GitHub, and whether the commit
# is there, as a status goes only on a commit that GitHub has
SLUG=""
ON_GITHUB=0
find_on_github() {
  SLUG=$(cd "${ROOT_DIR}" && gh repo view --json nameWithOwner --jq .nameWithOwner) ||
    die "gh does not find the repository on GitHub (gh auth login?)"
  if gh api "repos/${SLUG}/commits/${HEAD_SHA}" --silent >/dev/null 2>&1; then ON_GITHUB=1; fi
}

# Post on the commit the status ci-local/required in a state, success,
# pending or failure, whose description carries the fingerprint, the
# master of the merge and the week of the images, and then what the state
# says; the job local-ci of the CI takes only a success
post_status() {  # state, what follows the week
  local desc="fp=${FP} base=${BASE:0:12} img=${WEEK}; $2"
  desc=${desc:0:140}
  if (( DRY_RUN )); then
    say "would post on ${HEAD_SHA:0:12} the status ${STATUS_CONTEXT} ($1): ${desc}"
    return 0
  fi
  (( ON_GITHUB )) || return 0
  gh api -X POST "repos/${SLUG}/statuses/${HEAD_SHA}" -f "state=$1" \
    -f "context=${STATUS_CONTEXT}" -f "description=${desc}" --silent ||
    die "could not post the status on ${HEAD_SHA:0:12}"
  say "posted on ${HEAD_SHA:0:12} the status ${STATUS_CONTEXT} ($1): ${desc}"
}

# Post the success, with the versions of the compilers. The CI does not run
# again for a status: a local-ci that ended before it did not see it, and
# re-running that job re-runs the required jobs with it
publish() {
  if (( ! DRY_RUN && ! ON_GITHUB )); then
    say "${HEAD_SHA:0:12} is not on GitHub yet: push it and run again with --merge --publish, which posts this result without running the jobs"
    return 0
  fi
  post_status success "22.04: ${TOOLS[22.04]:-?}; 24.04: ${TOOLS[24.04]:-?}"
  (( DRY_RUN )) && return 0
  local run job
  run=$(gh api "repos/${SLUG}/actions/runs?head_sha=${HEAD_SHA}&event=pull_request&per_page=20" \
          --jq '[.workflow_runs[] | select(.name == "CI")][0].id // empty' 2>/dev/null) || run=""
  if [ -n "${run}" ]; then
    job=$(gh api "repos/${SLUG}/actions/runs/${run}/jobs?per_page=100" \
            --jq '.jobs[] | select(.name == "local-ci" and .status == "completed") | .id' 2>/dev/null) || job=""
    if [ -n "${job}" ]; then
      say "local-ci of the CI run ${run} already ended; if it did not take this status, re-run it, and the required jobs with it: gh run rerun --job ${job}"
    fi
  fi
}

cleanup() {
  trap - INT TERM
  say "stopping"
  docker ps --format '{{.Names}}' | grep -E "^aleph-ci-local-.*-$$\$" | xargs -r docker kill >/dev/null 2>&1 || true
  jobs -p | xargs -r kill 2>/dev/null || true
  exit 130
}
trap cleanup INT TERM

if [ -n "${SHOW}" ]; then
  [ -n "${KIND[${SHOW}]:-}" ] || die "unknown job: ${SHOW} (see --list)"
  if [ "${KIND[${SHOW}]}" = policy ]; then
    declare -f policy_job
  else
    job_script "${SHOW}"
  fi
  exit 0
fi

mkdir -p "${WORK}"
exec 9>"${WORK}/.lock"
flock -n 9 || die "another run uses ${WORK}"

STAMP="$(date +%Y%m%d-%H%M%S)"
LOGS="${WORK}/logs/${STAMP}"
mkdir -p "${LOGS}"
ln -sfn "${STAMP}" "${WORK}/logs/latest"

say "jobs: ${SELECTED[*]}"
say "${JOBS} at a time, ${NCPU} cores each; logs in ${LOGS}"
prepare_source
declare -A RELEASES=()
for j in "${SELECTED[@]}"; do
  if [ "${KIND[$j]}" != policy ]; then RELEASES[${REL[$j]}]=1; fi
done
for rel in "${!RELEASES[@]}"; do
  ensure_image "${rel}"
  say "image ${IMAGE[$rel]}: ${TOOLS[$rel]}"
done
if (( PUBLISH && ! DRY_RUN )); then find_on_github; fi

# A merge that passed is recorded by commit, master, fingerprint and week
# of the images, so that --publish after the push posts it without running
# the jobs again
RECORD=""
(( MERGE )) && RECORD="${WORK}/passed/${HEAD_SHA}-${BASE}-${FP}-${WEEK}"
recorded=0
if (( PUBLISH )) && [ -f "${RECORD}" ]; then
  recorded=1
  for j in "${SELECTED[@]}"; do
    if ! grep -qx "job ${j}" "${RECORD}"; then recorded=0; fi
  done
fi

if (( recorded )); then
  say "these jobs passed on this merge on $(date -r "${RECORD}" '+%Y-%m-%d %H:%M'): ${RECORD}"
else
  # a run of the same merge that fails must not leave the success before,
  # neither here nor in GitHub, where the status is pending while the jobs
  # run, and a failure if one fails
  if (( MERGE )); then rm -f "${RECORD}"; fi
  if (( PUBLISH )); then post_status pending "running the jobs"; fi
  running=0
  for j in "${SELECTED[@]}"; do
    while (( running >= JOBS )); do wait -n || true; running=$(( running - 1 )); done
    run_job "$j" &
    running=$(( running + 1 ))
  done
  wait || true

  failed=0
  printf '%-28s %-6s %6s  %s\n' job result time log > "${LOGS}/summary.txt"
  for j in "${SELECTED[@]}"; do
    read -r rc secs < "${LOGS}/$j.status" || { rc=1; secs=0; }
    if (( rc == 0 )); then res=PASS; else res=FAIL; failed=$(( failed + 1 )); fi
    printf '%-28s %-6s %3dm%02ds  %s\n' "$j" "$res" $(( secs / 60 )) $(( secs % 60 )) \
      "${LOGS}/$j.log" >> "${LOGS}/summary.txt"
  done
  cat "${LOGS}/summary.txt"
  if (( failed )); then
    say "${failed} of ${#SELECTED[@]} jobs failed; the failures:"
    for j in "${SELECTED[@]}"; do
      read -r rc _ < "${LOGS}/$j.status" || rc=1
      (( rc == 0 )) && continue
      echo "---- $j: $(describe "$j")"
      grep -E -A25 'The following tests FAILED|^FAIL |error:|Errors while running CTest' "${LOGS}/$j.log" | head -40 || tail -20 "${LOGS}/$j.log"
    done
    if (( PUBLISH )); then post_status failure "${failed} of ${#SELECTED[@]} jobs failed"; fi
    exit 1
  fi
  say "all ${#SELECTED[@]} jobs passed"
  if (( MERGE )); then
    mkdir -p "${WORK}/passed"
    { for j in "${SELECTED[@]}"; do echo "job ${j}"; done; cat "${LOGS}/summary.txt"; } > "${RECORD}"
  fi
fi

if (( PUBLISH )); then publish; fi
