#!/usr/bin/env ruby
# Aleph-w
# Data structures & Algorithms
# https://github.com/lrleon/Aleph-w
#
# Ruby frontend for the Python validator. Keeps CLI/API stable for Ruby-first
# workflows while delegating the heavy validation logic to the canonical script.


def command_in_path?(cmd)
  ENV.fetch("PATH", "").split(File::PATH_SEPARATOR).any? do |dir|
    path = File.join(dir, cmd)
    File.file?(path) and File.executable?(path)
  end
end


def resolve_python
  env_python = ENV.fetch("ALEPH_PYTHON", "").strip
  return env_python unless env_python.empty?
  return "python3" if command_in_path?("python3")
  return "python" if command_in_path?("python")
  nil
end


python = resolve_python
if python.nil?
  warn("No Python interpreter found (set ALEPH_PYTHON, or install python3).")
  exit(127)
end

script = File.expand_path("planarity_certificate_validator.py", __dir__)
exec(python, script, *ARGV)
