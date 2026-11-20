#!/usr/bin/env ruby
# frozen_string_literal: true

require 'minitest/autorun'
require 'open3'
require 'tmpdir'
require 'yaml'
require_relative 'ci_already_tested'

# Tests of already_tested(), against a fake of the API of GitHub
class TestCiAlreadyTested < Minitest::Test
  REPO = 'owner/repo'
  SHA = 'c' * 40
  HEAD = 'h' * 40
  TREE = 't' * 40

  def status(context: 'ci/tested-tree', state: 'success', poster: 'github-actions[bot]', tree: TREE)
    { 'context' => context, 'state' => state, 'creator' => { 'login' => poster }, 'description' => "tree #{tree}" }
  end

  # The answers of the API of GitHub for a squash SHA of the pull request 7,
  # whose head is HEAD, with the tree TREE
  def answers(prs: nil, statuses: nil)
    prs ||= [{ 'number' => 7, 'merged_at' => 'yes', 'merge_commit_sha' => SHA, 'head' => { 'sha' => HEAD } }]
    {
      "repos/#{REPO}/commits/#{SHA}/pulls" => prs,
      "repos/#{REPO}/git/commits/#{SHA}" => { 'tree' => { 'sha' => TREE } },
      "repos/#{REPO}/commits/#{HEAD}/statuses?per_page=100" => statuses || [status]
    }
  end

  def tested?(event: 'push', workflow: 'ci.yml', **overrides)
    table = answers(**overrides)
    api = ->(path) { table.fetch(path) { raise "no answer for #{path}" } }
    already_tested(event: event, repo: REPO, sha: SHA, workflow: workflow, api: api).first
  end

  def test_the_squash_of_a_pull_request_that_passed_on_the_same_tree_is_tested
    assert tested?
    assert tested?(workflow: 'ci-platform.yml', statuses: [status(context: 'ci-platform/tested-tree')])
  end

  def test_only_a_push_of_a_known_workflow_is_tested
    %w[pull_request schedule workflow_dispatch].each { |e| refute tested?(event: e), e }
    refute tested?(workflow: 'docs.yml')
  end

  def test_a_commit_that_is_not_the_merge_commit_of_a_merged_pull_request_is_not
    refute tested?(prs: [])
    refute tested?(prs: [{ 'number' => 7, 'merged_at' => nil, 'merge_commit_sha' => SHA, 'head' => { 'sha' => HEAD } }])
    refute tested?(prs: [{ 'number' => 7, 'merged_at' => 'yes', 'merge_commit_sha' => 'd' * 40,
                           'head' => { 'sha' => HEAD } }])
  end

  # The status must name the tree of the push: a run on the same head that
  # merged it into another branch tested another tree
  def test_only_the_status_of_this_tree_counts
    refute tested?(statuses: [])
    refute tested?(statuses: [status(tree: 'u' * 40)])
    assert tested?(statuses: [status(tree: 'u' * 40), status])
  end

  def test_only_a_status_of_success_of_github_actions_for_this_workflow_counts
    refute tested?(statuses: [status(state: 'failure')])
    refute tested?(statuses: [status(state: 'pending')])
    refute tested?(statuses: [status(poster: 'someone')])
    refute tested?(statuses: [status(context: 'ci-platform/tested-tree')])
    refute tested?(workflow: 'ci-platform.yml')
  end

  def test_a_pull_request_whose_required_jobs_the_local_ci_ran_is_not
    refute tested?(statuses: [status, { 'context' => 'ci-local/required', 'state' => 'success' }])
  end

  # 100 statuses fill a page of the API, which may leave one out
  def test_a_full_page_of_statuses_is_not
    refute tested?(statuses: [status] + Array.new(99) { status(context: 'codecov/patch') })
    assert tested?(statuses: [status] + Array.new(98) { status(context: 'codecov/patch') })
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

  # A push to master skips a job when the pull request recorded its tree, so
  # the job that records it waits for that job, checks its result, and
  # records nothing after a failure
  def test_the_tree_is_recorded_only_after_every_job_that_master_skips
    %w[ci.yml ci-platform.yml].each do |workflow|
      path = File.join(__dir__, '..', '.github', 'workflows', workflow)
      jobs = YAML.safe_load(File.read(path), aliases: true).fetch('jobs')
      skipped = jobs.select { |_, job| job['if'].to_s.include?('needs.already-tested.outputs.tested') }.keys
      recorders = jobs.select { |_, job| Array(job['steps']).any? { |step| step['run'].to_s.include?('tested-tree') } }
      refute_empty skipped, workflow
      refute_empty recorders, workflow
      recorders.each do |name, job|
        steps = job['steps']
        check = steps.index { |step| step['name'] == 'Check results' }
        record = steps.index { |step| step['run'].to_s.include?('tested-tree') }
        refute_nil check, "#{workflow}: #{name} records the tree without a step Check results"
        assert_operator check, :<, record, "#{workflow}: #{name} records the tree before it checks the results"
        refute_match(/always\(|failure\(|cancelled\(/, steps[record]['if'].to_s,
                     "#{workflow}: #{name} may record the tree after a failure")
        skipped.each do |job_name|
          assert_includes Array(job['needs']), job_name, "#{workflow}: #{name} does not wait for #{job_name}"
          assert_includes steps[check]['run'], %(check_job "#{job_name}" "${{ needs.#{job_name}.result }}"),
                          "#{workflow}: #{name} does not check #{job_name}"
        end
      end
    end
  end
end
