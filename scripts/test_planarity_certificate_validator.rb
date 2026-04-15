#!/usr/bin/env ruby
# frozen_string_literal: true
#
# Regression tests for planarity_certificate_validator.rb

require "minitest/autorun"
require "tmpdir"
require "pathname"
require "shellwords"
require "rbconfig"
require_relative "planarity_certificate_validator"

class TestPlanarityValidatorHelpers < Minitest::Test

  # -------------------------------------------------------------------------
  # normalize_ruby_cmd
  # -------------------------------------------------------------------------

  def test_normalize_ruby_cmd_replaces_leading_ruby
    cmd = "ruby -e \"puts 1\" {input}"
    result = normalize_ruby_cmd(cmd)
    assert result.start_with?(Shellwords.escape(RbConfig.ruby)),
           "Expected cmd to start with escaped RbConfig.ruby"
    assert_includes result, " -e \"puts 1\" {input}"
  end

  def test_normalize_ruby_cmd_leaves_non_ruby_cmd_unchanged
    cmd = "gephi --headless --import {input}"
    assert_equal cmd, normalize_ruby_cmd(cmd)
  end

  def test_normalize_ruby_cmd_does_not_alter_ruby_appearing_mid_string
    cmd = "gephi -f ruby_file.gexf"
    assert_equal cmd, normalize_ruby_cmd(cmd)
  end

  # -------------------------------------------------------------------------
  # host_platform
  # -------------------------------------------------------------------------

  def test_host_platform_returns_symbol
    result = host_platform
    assert_includes [:linux, :macos, :windows], result
  end

  # -------------------------------------------------------------------------
  # load_gephi_templates — catalog entries are normalized
  # -------------------------------------------------------------------------

  def test_load_gephi_templates_normalizes_ruby_cmd_from_catalog
    Dir.mktmpdir do |dir|
      catalog = Pathname.new(dir).join("templates.json")
      catalog.write(JSON.generate({
        "templates" => [
          {
            "id" => "test.ruby-probe",
            "os" => "any",
            "gephi_version" => "0.10.x",
            "description" => "test",
            "cmd" => "ruby -e \"puts 1\" {input}"
          }
        ]
      }))

      templates, warnings = load_gephi_templates(catalog)
      assert_empty warnings
      assert templates.key?("test.ruby-probe"), "Template should be loaded"
      cmd = templates["test.ruby-probe"]["cmd"]
      assert cmd.start_with?(Shellwords.escape(RbConfig.ruby)),
             "Catalog cmd should have leading 'ruby' replaced with RbConfig.ruby"
    end
  end

  # -------------------------------------------------------------------------
  # Empty --gephi-cmd triggers PATH probing via smoke template auto-selection
  # -------------------------------------------------------------------------

  def test_empty_gephi_cmd_triggers_path_probing
    Dir.mktmpdir do |dir|
      # Write a minimal valid GraphML file
      input = Pathname.new(dir).join("test.graphml")
      input.write(<<~XML)
        <?xml version="1.0" encoding="UTF-8"?>
        <graphml xmlns="http://graphml.graphdrawing.org/graphml">
          <graph id="G" edgedefault="undirected">
          </graph>
        </graphml>
      XML

      # Ensure the smoke probe template command is the portable.ruby-file-exists
      # template (which always succeeds if the file exists) by using the default
      # in-memory catalog (no catalog file present → built-in DEFAULT_GEPHI_TEMPLATES).
      missing_catalog = Pathname.new(dir).join("nonexistent_catalog.json")
      args = {
        "gephi"                 => true,
        "gephi_cmd"             => "",
        "gephi_template"        => "",
        "gephi_template_catalog"=> missing_catalog.to_s,
        "require_gephi"         => false
      }

      # Simulate the auto-selection logic that main() applies
      catalog_path = Pathname.new(args["gephi_template_catalog"])
      catalog_warnings = []
      if args["gephi"] and args["gephi_cmd"].empty? and args["gephi_template"].empty?
        default_template, default_warnings = find_default_smoke_probe_template(catalog_path)
        catalog_warnings.concat(default_warnings)
        if default_template
          args["gephi_template"] = default_template.fetch("id", "")
        end
      end

      refute_empty args["gephi_template"],
                   "Expected a smoke probe template to be auto-selected when --gephi-cmd is empty"
      assert_includes args["gephi_template"], "smoke",
                      "Auto-selected template should be a smoke probe"
    end
  end
end