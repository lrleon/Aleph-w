# The local CI and the hybrid CI

`scripts/ci-local.sh` runs on a Linux machine, in Docker containers, the
Linux jobs of `.github/workflows/ci.yml` that decide most pull requests:
the eight `build-and-test` jobs and `sanitizers`, which the ruleset of
`master` requires (the group `required`, the default), and on request the
other sanitizers, the graph harness and the policy gates. Each job runs in
an image of the Ubuntu of its job in the CI, with the packages, the
compilers and the CMake options of that job. `scripts/ci-local.sh --help`
and `--list` give the options and the jobs.

With it, a pull request can skip in GitHub the required Linux jobs that
already passed on the machine of a maintainer: the hybrid CI.

## How it works

1. `scripts/ci-local.sh --merge` tests what the CI tests for a pull
   request: the merge of the commit into the `master` of GitHub. It runs
   only as the `scripts/ci-local.sh` of that merge, whose jobs the
   fingerprint names: commit the changes to the script, and bring
   `master` into the branch if `master` changed it.
2. With `--publish`, when the required jobs pass, it posts on the commit
   the status `ci-local/required`, whose description carries:
   - `fp=`, the fingerprint of `ci.yml` and of `scripts/ci-local.sh` in the
     merge;
   - `base=`, the `master` of the merge;
   - `img=`, the week of the images, and the versions of their compilers.

   It needs Docker on x86_64, as the runners of the CI.
3. In the CI, the job `local-ci` looks for that status, and accepts it only
   - on a pull request of this repository, never on one from a fork;
   - with the label `ci-local`;
   - posted by a trusted user: the repository variable `CI_LOCAL_TRUSTED`,
     a list of logins separated by commas, by default the owner of the
     repository;
   - with the fingerprint and the `master` of the merge that the CI tests;
   - from images of this week or of the last.

   It waits for the status up to `CI_LOCAL_WAIT_MINUTES` (a repository
   variable, 20 by default and at most 25, within the timeout of the
   job), as long as the pull request keeps the label.
4. If it accepts it, the `build-and-test` jobs and `sanitizers` skip their
   steps and pass, so that the required checks of `master` report as
   always. Otherwise, or on any error of `local-ci`, they run in GitHub.

Everything else runs in GitHub: the policy gates, coverage, the other
sanitizers, the graph harness, the examples, macOS, Windows and Linux on
arm64. A push to `master` runs every job in GitHub.

## Use

Once, create the label (a maintainer):

```bash
gh label create ci-local --description "The required Linux jobs run in the local CI"
```

Open the pull request with the label `ci-local` (`gh pr create --label
ci-local`, or the labels of its form), so that its first run of the CI
waits for the status, and then:

```bash
scripts/ci-local.sh --merge             # before pushing: the required jobs on the merge
git push
scripts/ci-local.sh --merge --publish   # after it: posts the result of the run above
```

A result that passed for the same commit, `master`, workflow and week of
the images is posted without running the jobs again; without the first
run, `--publish` runs them, and the status is `pending` while they run,
and a `failure` if one fails, so that the success of an earlier run of the
same merge does not stand. `--dry-run` prints the statuses instead of
posting them. If `master` moves before the CI starts, the status no longer
matches: run `scripts/ci-local.sh --merge --publish` again.

The CI does not run again when a label is added or a status posted: if
`local-ci` ended before them, re-run that job, which re-runs the required
jobs with it: `gh run rerun --job <id>`, which `--publish` prints then, or
the button of the job in GitHub.

## When the required jobs run in GitHub

- The pull request has no label `ci-local`, or comes from a fork.
- No status of a trusted user arrives within the wait.
- The status is of another merge: `master` moved, or the merge changes
  `ci.yml` or `scripts/ci-local.sh` (the fingerprint); or its images are
  older than last week.
- The label is taken off while `local-ci` waits.
- `local-ci` fails, or GitHub does not answer it.

## Keeping the local CI in step with GitHub

The jobs run inside the images, with their compilers and libraries: the
distribution and the compilers of the machine do not enter. What can
drift, and what keeps it in step:

- **The packages.** The CI installs the latest ones in every run. The
  images are built anew every week, without the cache of Docker, from the
  latest base image and packages; the first run of a week builds them,
  and a status names the week of its images.
- **The definition of the jobs.** `scripts/ci-local.sh` copies the jobs of
  `ci.yml` by hand. The fingerprint covers both files, so that a change to
  either needs a new local run, and the jobs in `ci.yml` and in the script
  must change together.
- **The machine.** Docker, the kernel, which the containers share (it
  changes the behaviour of the sanitizers), and the architecture, which
  must be x86_64: `--publish` refuses another one. After changing the
  machine, Docker or the kernel, run a
  commit both in the local CI and in GitHub, and compare, before trusting
  the local CI again.
- **The policy job** of `scripts/ci-local.sh all` runs on the machine,
  with its Ruby and its Doxygen (the CI uses Doxygen 1.9.8), on the merge
  with `--merge`; it always runs in GitHub too.

A push to `master` runs every job in GitHub, so that a drift shows up at
the first merge.

## Cost

On a machine of 20 cores, the first run of `required` builds the images
and every job from scratch in some 23 minutes, and a later one takes 5 to
8 minutes; the CI takes 17. The jobs keep some 35 GB of build directories
and ccache in `~/.cache/aleph-ci-local`. In GitHub, a pull request whose
status the CI accepts saves the runs of the nine required jobs, and the
caches of ccache that they save, in the 10 GB of Actions cache of the
repository.
