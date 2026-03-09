#!/usr/bin/env ruby
# frozen_string_literal: true

require 'json'
require 'open3'
require 'optparse'
require 'pathname'
require 'tempfile'

options = {
  build_dir: 'build-test-clean',
  probe: nil
}

parser = OptionParser.new do |opts|
  opts.banner = 'Usage: fft_reference_check.rb [--build-dir DIR] [--probe PATH]'
  opts.on('--build-dir DIR', 'Build directory that contains Examples/fft_reference_probe') do |value|
    options[:build_dir] = value
  end
  opts.on('--probe PATH', 'Explicit path to fft_reference_probe') do |value|
    options[:probe] = value
  end
end
parser.parse!(ARGV)

repo_root = Pathname(__dir__).parent
probe = options[:probe] ?
          Pathname(options[:probe]).expand_path :
          repo_root.join(options[:build_dir], 'Examples', 'fft_reference_probe')

unless probe.exist?
  warn "fft_reference_probe not found at #{probe}. Build it first with:"
  warn "  cmake --build #{options[:build_dir]} -j4 --target fft_reference_probe"
  exit 1
end

_stdout, _stderr, status = Open3.capture3('python3', '-c', 'import numpy')
unless status.success?
  warn 'NumPy is not available; external FFT cross-validation was skipped.'
  exit 0
end

probe_json, probe_err, probe_status = Open3.capture3([probe.to_s, probe.to_s])
unless probe_status.success?
  warn probe_err
  exit probe_status.exitstatus || 1
end

payload = JSON.parse(probe_json)
python_script = <<~PY
  import json
  import numpy as np
  import sys

  payload = json.load(sys.stdin)

  def as_complex(values):
      return np.array([complex(re, im) for re, im in values], dtype=np.complex128)

  def as_real(values):
      return np.array(values, dtype=np.float64)

  def max_error(lhs, rhs):
      return float(np.max(np.abs(lhs - rhs))) if len(lhs) else 0.0

  complex_signal = as_complex(payload['complex_signal'])
  complex_spectrum = as_complex(payload['complex_spectrum'])
  real_signal = as_real(payload['real_signal'])
  real_full_spectrum = as_complex(payload['real_full_spectrum'])
  real_compact_spectrum = as_complex(payload['real_compact_spectrum'])
  real_reconstructed = as_real(payload['real_reconstructed'])
  prime_signal = as_complex(payload['prime_signal'])
  prime_spectrum = as_complex(payload['prime_spectrum'])

  results = {
      'complex_fft': max_error(complex_spectrum, np.fft.fft(complex_signal)),
      'real_full_fft': max_error(real_full_spectrum, np.fft.fft(real_signal)),
      'real_compact_fft': max_error(real_compact_spectrum, np.fft.rfft(real_signal)),
      'real_irfft': max_error(real_reconstructed, np.fft.irfft(real_compact_spectrum, n=real_signal.shape[0])),
      'prime_fft': max_error(prime_spectrum, np.fft.fft(prime_signal)),
  }

  print(json.dumps(results))
PY

results_json, py_err, py_status = Open3.capture3('python3', '-c', python_script, stdin_data: JSON.generate(payload))
unless py_status.success?
  warn py_err
  exit py_status.exitstatus || 1
end

results = JSON.parse(results_json)
threshold = 5.0e-10
failed = false

puts 'FFT external cross-validation against NumPy'
results.each do |name, value|
  printf("  %-18s %.6e\n", name, value)
  failed ||= value > threshold
end
puts format('  threshold           %.6e', threshold)

exit(failed ? 1 : 0)
