#!/usr/bin/env ruby
# frozen_string_literal: true

require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require_relative 'ci_already_tested'

# Tests of already_tested(), against a fake of the API of GitHub
class TestCiAlreadyTested < Minitest::Test
  REPO = 'owner/repo'
  SHA = 'c' * 40
  HEAD = 'h' * 40
  PASSED = { 'run_number' => 2, 'status' => 'completed', 'conclusion' => 'success', 'html_url' => 'url' }.freeze

  # The answers of the API of GitHub for a squash SHA of the pull request 7,
  # whose head is HEAD
  def answers(prs: nil, head_tree: 'tree', statuses: [], runs: [PASSED])
    prs ||= [{ 'number' => 7, 'merged_at' => 'yes', 'merge_commit_sha' => SHA, 'head' => { 'sha' => HEAD } }]
    {
      "repos/#{REPO}/commits/#{SHA}/pulls" => prs,
      "repos/#{REPO}/git/commits/#{SHA}" => { 'tree' => { 'sha' => 'tree' } },
      "repos/#{REPO}/git/commits/#{HEAD}" => { 'tree' => { 'sha' => head_tree } },
      "repos/#{REPO}/commits/#{HEAD}/statuses?per_page=100" => statuses,
      "repos/#{REPO}/actions/workflows/ci.yml/runs?event=pull_request&head_sha=#{HEAD}&per_page=20" =>
        { 'workflow_runs' => runs }
    }
  end

  def tested?(event: 'push', **overrides)
    table = answers(**overrides)
    api = ->(path) { table.fetch(path) { raise "no answer for #{path}" } }
    already_tested(event: event, repo: REPO, sha: SHA, workflow: 'ci.yml', api: api).first
  end

  def test_the_squash_of_a_pull_request_that_passed_on_the_same_tree_is_tested
    assert tested?
  end

  def test_only_a_push_is_tested
    %w[pull_request schedule workflow_dispatch].each { |e| refute tested?(event: e), e }
  end

  def test_a_commit_that_is_not_the_merge_commit_of_a_merged_pull_request_is_not
    refute tested?(prs: [])
    refute tested?(prs: [{ 'number' => 7, 'merged_at' => nil, 'merge_commit_sha' => SHA, 'head' => { 'sha' => HEAD } }])
    refute tested?(prs: [{ 'number' => 7, 'merged_at' => 'yes', 'merge_commit_sha' => 'd' * 40,
                           'head' => { 'sha' => HEAD } }])
  end

  def test_a_head_with_another_tree_is_not
    refute tested?(head_tree: 'other tree')
  end

  def test_a_pull_request_whose_required_jobs_the_local_ci_ran_is_not
    refute tested?(statuses: [{ 'context' => 'ci-local/required', 'state' => 'success' }])
    assert tested?(statuses: [{ 'context' => 'codecov/patch', 'state' => 'success' }])
    # 100 statuses fill a page of the API, which may leave ci-local/required
    # out: it says no
    refute tested?(statuses: Array.new(100) { { 'context' => 'codecov/patch', 'state' => 'success' } })
    assert tested?(statuses: Array.new(99) { { 'context' => 'codecov/patch', 'state' => 'success' } })
  end

  def test_the_last_run_must_have_passed_and_no_other_failed
    refute tested?(runs: [])
    refute tested?(runs: [{ 'run_number' => 3, 'status' => 'completed', 'conclusion' => 'failure' }, PASSED])
    refute tested?(runs: [{ 'run_number' => 3, 'status' => 'in_progress', 'conclusion' => nil }, PASSED])
    # Two pull requests may share a head: the failed run may be that of the
    # merged one, and the runs do not say for which pull request they ran
    refute tested?(runs: [{ 'run_number' => 1, 'status' => 'completed', 'conclusion' => 'failure' }, PASSED])
    refute tested?(runs: [{ 'run_number' => 1, 'status' => 'in_progress', 'conclusion' => nil }, PASSED])
    refute tested?(runs: [{ 'run_number' => 1, 'status' => 'completed', 'conclusion' => 'cancelled' }, PASSED])
    assert tested?(runs: [{ 'run_number' => 1, 'status' => 'completed', 'conclusion' => 'success' }, PASSED])
    # A run without a number cannot be ordered, and a full page may hide one
    refute tested?(runs: [{ 'run_number' => nil, 'status' => 'completed', 'conclusion' => 'success' }, PASSED])
    refute tested?(runs: Array.new(20) { |i| PASSED.merge('run_number' => i + 1) })
    assert tested?(runs: Array.new(19) { |i| PASSED.merge('run_number' => i + 1) })
  end

  def test_an_error_of_the_api_is_not_tested
    api = ->(_path) { raise 'no network' }
    tested, reason = already_tested(event: 'push', repo: REPO, sha: SHA, workflow: 'ci.yml', api: api)
    refute tested
    assert_match(/no network/, reason)
  end

  # The script writes its answer for the next jobs, and never fails
  def test_the_script_writes_its_answer_and_succeeds
    Dir.mktmpdir do |dir|
      output = File.join(dir, 'output')
      env = { 'GITHUB_EVENT_NAME' => 'pull_request', 'GITHUB_OUTPUT' => output }
      out, status = Open3.capture2e(env, RbConfig.ruby, File.join(__dir__, 'ci_already_tested.rb'), 'ci.yml')
      assert status.success?, out
      assert_equal "tested=false\n", File.read(output)
    end
  end
end
