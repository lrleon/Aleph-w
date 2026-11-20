#!/usr/bin/env ruby
# frozen_string_literal: true

# Whether the pull request that a push to master merges already ran a
# workflow, with success, on the tree of the push, so that the push can
# skip the jobs that the pull request ran (docs/ci-local.md).
#
# Use: ruby scripts/ci_already_tested.rb <workflow file>
#
# It reads GITHUB_EVENT_NAME, GITHUB_REPOSITORY and GITHUB_SHA, asks GitHub
# with gh (GH_TOKEN), prints why, and appends tested=true or tested=false to
# GITHUB_OUTPUT. It never fails: on any doubt, or on any error, it says
# false, and the jobs run.
#
# A push to master is the squash of a pull request, and master requires a
# branch to be up to date before it merges: the head of the pull request
# then holds the tree that the squash leaves on master, and the last run of
# the workflow on that head, on the merge of the head into master, checked
# out that same tree. It says true only if
#   - the event is a push, and the commit is the merge commit of a merged
#     pull request;
#   - the head of that pull request has the same tree as the commit;
#   - the head has no status ci-local/required: when the local CI ran the
#     required jobs of a pull request instead of GitHub, master runs them
#     all, so that a drift between the local CI and GitHub shows up at the
#     first merge;
#   - the last run of the workflow on that head, for a pull request,
#     completed with success, and every other run of it on that head
#     passed too: the runs of a commit do not say for which pull
#     request they ran, and two pull requests may share a head, so that a
#     failed run may be that of the merged one.

require 'json'
require 'open3'

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

  pr = api.call("repos/#{repo}/commits/#{sha}/pulls")
          .find { |p| p['merged_at'] && p['merge_commit_sha'] == sha }
  return [false, "#{sha[0, 12]} is not the merge commit of a pull request"] unless pr

  number = pr['number']
  head = pr.dig('head', 'sha').to_s
  tree = api.call("repos/#{repo}/git/commits/#{sha}").dig('tree', 'sha')
  head_tree = api.call("repos/#{repo}/git/commits/#{head}").dig('tree', 'sha')
  return [false, "the head of ##{number}, #{head[0, 12]}, has another tree"] unless tree && tree == head_tree

  statuses = api.call("repos/#{repo}/commits/#{head}/statuses?per_page=100")
  # A full page may leave ci-local/required on the next one
  return [false, "the head of ##{number} has too many statuses to read"] if statuses.size >= 100
  if statuses.any? { |s| s['context'] == 'ci-local/required' }
    return [false, "the local CI ran the required jobs of ##{number}: they run here"]
  end

  answer = api.call("repos/#{repo}/actions/workflows/#{workflow}/runs?event=pull_request&head_sha=#{head}&per_page=20")
  runs = Array(answer['workflow_runs'])
  return [false, "a run of #{workflow} on ##{number} has no number"] if runs.any? { |r| r['run_number'].nil? }
  return [false, "the head of ##{number} has too many runs of #{workflow} to read"] if runs.size >= 20

  run = runs.max_by { |r| r['run_number'] }
  return [false, "no run of #{workflow} on the head of ##{number}"] unless run

  unless run['status'] == 'completed' && run['conclusion'] == 'success'
    return [false, "the last run of #{workflow} on ##{number} is #{run['status']} #{run['conclusion']}"]
  end

  other = runs.find { |r| r['status'] != 'completed' || !%w[success skipped].include?(r['conclusion']) }
  return [false, "run #{other['run_number']} of #{workflow} on that head is #{other['status']} #{other['conclusion']}"] if other

  [true, "##{number} ran #{workflow} with success on this tree: #{run['html_url']}"]
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
