#!/usr/bin/env ruby
# frozen_string_literal: true

# Whether the pull request that a push to master merges already passed a
# workflow on the tree of the push, so that the push can skip the jobs that
# the pull request ran (docs/ci-local.md).
#
# Use: ruby scripts/ci_already_tested.rb <workflow file>
#
# It reads GITHUB_EVENT_NAME, GITHUB_REPOSITORY and GITHUB_SHA, asks GitHub
# with gh (GH_TOKEN), prints why, and appends tested=true or tested=false to
# GITHUB_OUTPUT. It never fails: on any doubt, or on any error, it says
# false, and the jobs run.
#
# When a run of the workflow passes on a pull request, its last job posts on
# the head of the pull request the status ci/tested-tree (ci-platform.yml:
# ci-platform/tested-tree), whose description names the tree of the merge
# that the run checked out. That binds the run to the tree that it tested,
# which the runs themselves do not say: a commit may head two pull requests,
# into different branches. It says true only if
#   - the event is a push, and the commit is the merge commit of a merged
#     pull request from a branch of this repository: a fork cannot post
#     the status, and its head may be the head of a pull request of the
#     repository that posted it;
#   - the head of that pull request has that status, posted by GitHub
#     Actions, with success, naming the tree of the commit;
#   - the head has no status ci-local/required: when the local CI ran the
#     required jobs of a pull request instead of GitHub, master runs them
#     all, so that a drift between the local CI and GitHub shows up at the
#     first merge.

require 'json'
require 'open3'

# The status that the last job of each workflow posts when it passes
TESTED_TREE = { 'ci.yml' => 'ci/tested-tree', 'ci-platform.yml' => 'ci-platform/tested-tree' }.freeze

# Who posts it: the token of GitHub Actions
POSTER = 'github-actions[bot]'

# Asks the API of GitHub for path, with gh, and returns the parsed JSON.
def gh_api(path)
  out, err, status = Open3.capture3('gh', 'api', path)
  raise "gh api #{path}: #{err.strip}" unless status.success?

  JSON.parse(out)
end

# Returns [tested, reason], with api answering the paths of the API of
# GitHub (gh_api, or a fake in the tests).
def already_tested(event:, repo:, sha:, workflow:, api: method(:gh_api))
  return [false, "a #{event} event, not a push"] unless event == 'push'

  context = TESTED_TREE[workflow] or return [false, "no status of a tested tree for #{workflow}"]
  pr = api.call("repos/#{repo}/commits/#{sha}/pulls")
          .find { |p| p['merged_at'] && p['merge_commit_sha'] == sha }
  return [false, "#{sha[0, 12]} is not the merge commit of a pull request"] unless pr

  number = pr['number']
  return [false, "##{number} comes from a fork"] unless pr.dig('head', 'repo', 'full_name') == repo

  head = pr.dig('head', 'sha').to_s
  tree = api.call("repos/#{repo}/git/commits/#{sha}").dig('tree', 'sha')
  return [false, "no tree for #{sha[0, 12]}"] unless tree

  statuses = api.call("repos/#{repo}/commits/#{head}/statuses?per_page=100")
  # A full page may leave a status on the next one
  return [false, "the head of ##{number} has too many statuses to read"] if statuses.size >= 100
  if statuses.any? { |s| s['context'] == 'ci-local/required' }
    return [false, "the local CI ran the required jobs of ##{number}: they run here"]
  end

  passed = statuses.find do |s|
    s['context'] == context && s['state'] == 'success' && s.dig('creator', 'login') == POSTER &&
      s['description'] == "tree #{tree}"
  end
  return [false, "no #{context} of ##{number} names the tree #{tree[0, 12]}"] unless passed

  [true, "##{number} passed #{workflow} on this tree, #{tree[0, 12]}"]
rescue StandardError => e
  [false, "an error: #{e.message}"]
end

if $PROGRAM_NAME == __FILE__
  tested, reason = already_tested(event: ENV.fetch('GITHUB_EVENT_NAME', ''),
                                  repo: ENV.fetch('GITHUB_REPOSITORY', ''),
                                  sha: ENV.fetch('GITHUB_SHA', ''),
                                  workflow: ARGV.fetch(0, ''))
  puts "#{tested ? 'Already tested' : 'Not tested'}: #{reason}"
  output = ENV.fetch('GITHUB_OUTPUT', nil)
  File.open(output, 'a') { |f| f.puts "tested=#{tested}" } if output
end
