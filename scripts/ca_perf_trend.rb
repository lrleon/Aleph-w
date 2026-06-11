#!/usr/bin/env ruby
# frozen_string_literal: true

# Aleph-w — render the cellular-automata perf trend as a dependency-free page.
#
# Reads the JSON-lines history produced by ca_perf_regression.rb
# --history-append, keeps the most recent window (90 days by default), and emits
# a self-contained HTML page with an inline SVG line chart. Each benchmark is
# normalised to its first in-window sample so the very different absolute wall
# times (sub-second to multi-second) share one percentage axis.

require 'json'
require 'optparse'
require 'fileutils'
require 'time'

PALETTE = %w[#1f77b4 #d62728 #2ca02c #9467bd #ff7f0e #17becf #8c564b #e377c2].freeze

W = 960
H = 480
PAD_L = 60
PAD_R = 200
PAD_T = 30
PAD_B = 50
PLOT_W = W - PAD_L - PAD_R
PLOT_H = H - PAD_T - PAD_B

options = { history: 'benchmarks/history.jsonl', days: 90, out_dir: 'benchmarks/trend-site' }
OptionParser.new do |opts|
  opts.banner = 'Usage: ca_perf_trend.rb [options]'
  opts.on('--history PATH', 'JSON-lines history file') { |v| options[:history] = v }
  opts.on('--days N', Integer, 'Trailing window in days (default 90)') { |v| options[:days] = v }
  opts.on('--out-dir DIR', 'Output directory for the page') { |v| options[:out_dir] = v }
end.parse!(ARGV)

def load_history(path, days)
  return [] unless File.exist?(path)

  cutoff = Time.now.utc - days * 86_400
  records = []
  File.foreach(path) do |line|
    line = line.strip
    next if line.empty?

    rec = JSON.parse(line)
    when_t = Time.parse(rec['utc'])
    rec['_when'] = when_t
    records << rec if when_t >= cutoff
  end
  records.sort_by { |r| r['_when'] }
end

def series_from(records)
  names = records.flat_map { |r| r['benchmarks'].keys }.uniq.sort
  return [names, {}] if records.empty?

  span = (records.last['_when'] - records.first['_when']).to_f
  span = 1.0 if span <= 0
  series = {}
  names.each do |name|
    pts = []
    first = nil
    records.each do |r|
      next unless r['benchmarks'].key?(name)

      value = r['benchmarks'][name]
      first = (value.zero? ? 1 : value) if first.nil?
      x = (r['_when'] - records.first['_when']).to_f / span
      pts << [x, 100.0 * value / first]
    end
    series[name] = pts unless pts.empty?
  end
  [names, series]
end

def svg(records, series)
  if series.empty?
    return %(<svg xmlns="http://www.w3.org/2000/svg" width="#{W}" height="80">) +
           %(<text x="20" y="45" font-family="sans-serif">No perf history yet.</text></svg>)
  end

  all_y = series.values.flatten(1).map { |_, y| y }
  y_lo = [90.0, all_y.min].min
  y_hi = [110.0, all_y.max].max

  sx = ->(xf) { PAD_L + xf * PLOT_W }
  sy = ->(yv) { PAD_T + (y_hi - yv) / (y_hi - y_lo) * PLOT_H }

  parts = [%(<svg xmlns="http://www.w3.org/2000/svg" width="#{W}" height="#{H}" font-family="sans-serif" font-size="12">)]
  parts << %(<rect width="#{W}" height="#{H}" fill="white"/>)
  (0..4).each do |k|
    yv = y_lo + (y_hi - y_lo) * k / 4
    y = sy.call(yv)
    parts << format('<line x1="%d" y1="%.1f" x2="%d" y2="%.1f" stroke="#eee"/>', PAD_L, y, PAD_L + PLOT_W, y)
    parts << format('<text x="%d" y="%.1f" text-anchor="end" fill="#666">%.0f%%</text>', PAD_L - 8, y + 4, yv)
  end
  y100 = sy.call(100.0)
  parts << format('<line x1="%d" y1="%.1f" x2="%d" y2="%.1f" stroke="#bbb" stroke-dasharray="4 3"/>', PAD_L, y100, PAD_L + PLOT_W, y100)
  parts << format('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#333"/>', PAD_L, PAD_T, PAD_L, PAD_T + PLOT_H)
  parts << format('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="#333"/>', PAD_L, PAD_T + PLOT_H, PAD_L + PLOT_W, PAD_T + PLOT_H)
  parts << format('<text x="%d" y="%d" text-anchor="middle" fill="#333">%s → %s (%d samples)</text>',
                  PAD_L + PLOT_W / 2, H - 12,
                  records.first['_when'].strftime('%Y-%m-%d'),
                  records.last['_when'].strftime('%Y-%m-%d'), records.size)
  parts << format('<text transform="translate(16,%d) rotate(-90)" text-anchor="middle" fill="#333">wall time vs. first sample</text>',
                  PAD_T + PLOT_H / 2)
  series.sort.each_with_index do |(name, pts), i|
    color = PALETTE[i % PALETTE.size]
    poly = pts.map { |x, y| format('%.1f,%.1f', sx.call(x), sy.call(y)) }.join(' ')
    parts << %(<polyline points="#{poly}" fill="none" stroke="#{color}" stroke-width="2"/>)
    pts.each do |x, y|
      parts << format('<circle cx="%.1f" cy="%.1f" r="2.2" fill="%s"/>', sx.call(x), sy.call(y), color)
    end
    ly = PAD_T + 16 + i * 20
    parts << format('<line x1="%d" y1="%d" x2="%d" y2="%d" stroke="%s" stroke-width="3"/>',
                    PAD_L + PLOT_W + 12, ly, PAD_L + PLOT_W + 32, ly, color)
    parts << format('<text x="%d" y="%d" fill="#333">%s</text>', PAD_L + PLOT_W + 38, ly + 4, name)
  end
  parts << '</svg>'
  parts.join
end

records = load_history(options[:history], options[:days])
_, series = series_from(records)
chart = svg(records, series)
FileUtils.mkdir_p(options[:out_dir])
File.write(File.join(options[:out_dir], 'trend.svg'), chart)
generated = Time.now.utc.strftime('%Y-%m-%d %H:%M UTC')
html = "<!doctype html><html><head><meta charset='utf-8'>" \
       "<title>Aleph-w CA perf trend</title></head><body>" \
       "<h1>Cellular-automata performance trend (#{options[:days]} days)</h1>" \
       "<p>Generated #{generated}. Lower is better; each series is normalised " \
       "to its first sample in the window.</p>#{chart}</body></html>\n"
File.write(File.join(options[:out_dir], 'index.html'), html)
puts "Wrote trend page for #{records.size} samples to #{options[:out_dir]}"