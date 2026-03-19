#!/usr/bin/env ruby
# frozen_string_literal: true

require 'json'
require 'open3'
require 'optparse'
require 'pathname'
require 'shellwords'

options = {
  build_dir: 'build',
  python: 'python3'
}

parser = OptionParser.new do |opts|
  opts.banner = <<~USAGE
    Usage:
      polynomial_reference_check.rb [--build-dir DIR]
      polynomial_reference_check.rb --probe /path/to/polynomial_reference_probe [--python python3]
  USAGE

  opts.on('--build-dir DIR', 'Build directory containing Examples/polynomial_reference_probe') do |value|
    options[:build_dir] = value
  end

  opts.on('--probe PATH', 'Explicit path to polynomial_reference_probe') do |value|
    options[:probe] = value
  end

  opts.on('--python PATH', 'Python interpreter to use (default: python3)') do |value|
    options[:python] = value
  end
end
parser.parse!(ARGV)

repo_root = Pathname(__dir__).join('..').expand_path
probe = if options[:probe]
          Pathname(options[:probe]).expand_path
        else
          repo_root.join(options[:build_dir], 'Examples', 'polynomial_reference_probe')
        end

unless probe.file? && probe.executable?
  warn "polynomial_reference_probe not found or not executable at #{probe}. Build it first with:"
  warn "  cmake --build #{options[:build_dir]} -j4 --target polynomial_reference_probe"
  exit 1
end

sympy_check = <<~'PY'
  try:
      import sympy  # noqa: F401
  except Exception:
      raise SystemExit(2)
PY

_stdout, _stderr, status = Open3.capture3(options[:python], '-c', sympy_check)
unless status.success?
  puts "[skip] SymPy is not available for differential checking."
  exit 0
end

probe_stdout, probe_stderr, probe_status = Open3.capture3(probe.to_s, '--json')
unless probe_status.success?
  warn probe_stderr
  abort("Probe command failed: #{[probe.to_s, '--json'].shelljoin}")
end

python_script = <<~'PY'
  import json
  import sys
  import sympy as sp

  payload = json.load(sys.stdin)
  if payload.get("mode") != "reference":
      raise SystemExit("expected mode=reference")

  x, y = sp.symbols("x y")

  def uni_poly(coeffs):
      expr = sp.Integer(0)
      for exp, coeff in enumerate(coeffs):
          expr += sp.Integer(coeff) * x**exp
      return sp.expand(expr)

  def term_poly(terms, vars_):
      expr = sp.Integer(0)
      for term in terms:
          coeff = sp.Integer(term["coeff"])
          mon = coeff
          for var, exp in zip(vars_, term["exponents"]):
              mon *= var**int(exp)
          expr += mon
      return sp.expand(expr)

  def normalize_poly(expr, vars_):
      if isinstance(expr, sp.Poly):
          poly = sp.Poly(expr.as_expr(), *vars_, domain="ZZ")
      else:
          poly = sp.Poly(sp.expand(expr), *vars_, domain="ZZ")
      _, primitive = poly.primitive()
      if primitive.LC() < 0:
          primitive = -primitive
      return tuple((tuple(mon), int(coeff)) for mon, coeff in primitive.terms())

  def normalize_factor_list(factors, vars_):
      normalized = []
      for item in factors:
          factor_expr = uni_poly(item["factor"]) if vars_ == (x,) else term_poly(item["factor"], vars_)
          key = normalize_poly(factor_expr, vars_)
          normalized.append((key, int(item["multiplicity"])))
      normalized.sort()
      return normalized

  for case in payload["univariate"]:
      if case["name"] == "sum_mul_divmod":
          a = uni_poly(case["a"])
          b = uni_poly(case["b"])
          expected_sum = uni_poly(case["sum"])
          if sp.expand(a + b) != expected_sum:
              raise AssertionError("sum_mul_divmod: incorrect sum")
          expected_product = uni_poly(case["product"])
          if sp.expand(a * b) != expected_product:
              raise AssertionError("sum_mul_divmod: incorrect product")
          q = uni_poly(case["quotient"])
          r = uni_poly(case["remainder"])
          if sp.expand(q * b + r) != expected_product:
              raise AssertionError("sum_mul_divmod: quotient * divisor + remainder mismatch")
      elif case["name"] == "compose_sparse":
          outer = uni_poly(case["outer"])
          inner = uni_poly(case["inner"])
          if sp.expand(outer.subs(x, inner)) != uni_poly(case["composed"]):
              raise AssertionError("compose_sparse: incorrect composition")
      elif case["name"] == "factorize_integer":
          poly = uni_poly(case["poly"])
          library = normalize_factor_list(case["factors"], (x,))
          content, factors = sp.factor_list(poly)
          expected = []
          if content != 1:
              expected.append((normalize_poly(content, (x,)), 1))
          for factor, multiplicity in factors:
              expected.append((normalize_poly(factor.as_expr(), (x,)), int(multiplicity)))
          expected.sort()
          if library != expected:
              raise AssertionError((library, expected))
      else:
          raise AssertionError(f"unknown univariate case {case['name']}")

  for case in payload["multivariate"]:
      if case["name"] == "factorize_difference_of_squares":
          poly = term_poly(case["poly"], (x, y))
          library = normalize_factor_list(case["factors"], (x, y))
          content, factors = sp.factor_list(poly)
          expected = []
          if content != 1:
              expected.append((normalize_poly(content, (x, y)), 1))
          for factor, multiplicity in factors:
              expected.append((normalize_poly(factor.as_expr(), (x, y)), int(multiplicity)))
          expected.sort()
          if library != expected:
              raise AssertionError((library, expected))
      elif case["name"] == "factorize_same_degree_non_monic":
          poly = term_poly(case["poly"], (x, y))
          library = normalize_factor_list(case["factors"], (x, y))
          content, factors = sp.factor_list(poly)
          expected = []
          if content != 1:
              expected.append((normalize_poly(content, (x, y)), 1))
          for factor, multiplicity in factors:
              expected.append((normalize_poly(factor.as_expr(), (x, y)), int(multiplicity)))
          expected.sort()
          if library != expected:
              raise AssertionError((library, expected))
      elif case["name"] == "groebner_reduced_lex":
          generators = [term_poly(poly_terms, (x, y)) for poly_terms in case["generators"]]
          basis = [term_poly(poly_terms, (x, y)) for poly_terms in case["basis"]]
          gb = sp.groebner(generators, x, y, order="lex")
          expected = sorted(normalize_poly(poly, (x, y)) for poly in gb.polys)
          got = sorted(normalize_poly(poly, (x, y)) for poly in basis)
          if got != expected:
              raise AssertionError((got, expected))
      else:
          raise AssertionError(f"unknown multivariate case {case['name']}")

  print("polynomial_reference_check: differential comparison passed")
PY

stdout, stderr, status =
  Open3.capture3(options[:python], '-c', python_script, stdin_data: probe_stdout)
unless status.success?
  warn stderr
  abort('Differential comparison against SymPy failed')
end

puts stdout
