/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file ca_bench_json_test.cc
 * @brief Validate that the perf-gate benchmark JSON is well formed.
 *
 * The performance gate parses each benchmark's single JSON line with an
 * ordinary JSON reader, so this test exercises the emitter with a genuine
 * recursive-descent parser: it must consume the entire line and expose the
 * fixed `{name, wall_ns, cells_per_sec, compiler, cpu}` schema with the
 * correct value types.
 */

#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <variant>

#include <gtest/gtest.h>

#include <benchmarks/ca/bench_support.H>

using Aleph::CA::Bench::Bench_Result;
using Aleph::CA::Bench::emit_json;

namespace
{

// Minimal but strict JSON reader: objects, strings and numbers are enough for
// the benchmark schema. A leftover non-whitespace tail makes parsing fail, so
// a malformed trailing byte cannot slip through.
struct Json_Value
{
  std::variant<std::string, double> data;

  [[nodiscard]] bool is_string() const { return data.index() == 0; }
  [[nodiscard]] bool is_number() const { return data.index() == 1; }
  [[nodiscard]] const std::string &str() const { return std::get<0>(data); }
  [[nodiscard]] double num() const { return std::get<1>(data); }
};

class Json_Parser
{
public:
  explicit Json_Parser(std::string text) : text_(std::move(text)) {}

  // Parse a single object and assert the whole string was consumed.
  bool parse_object(std::map<std::string, Json_Value> &out)
  {
    skip_ws();
    if (not eat('{'))
      return false;
    skip_ws();
    if (peek() == '}')
      {
        ++pos_;
        return tail_clean();
      }
    while (true)
      {
        skip_ws();
        std::string key;
        if (not parse_string(key))
          return false;
        skip_ws();
        if (not eat(':'))
          return false;
        Json_Value value;
        if (not parse_value(value))
          return false;
        out.emplace(std::move(key), std::move(value));
        skip_ws();
        if (eat(','))
          continue;
        if (eat('}'))
          break;
        return false;
      }
    return tail_clean();
  }

private:
  std::string text_;
  std::size_t pos_ = 0;

  char peek() const { return pos_ < text_.size() ? text_[pos_] : '\0'; }
  void skip_ws()
  {
    while (pos_ < text_.size()
           and (text_[pos_] == ' ' or text_[pos_] == '\t'
                or text_[pos_] == '\n' or text_[pos_] == '\r'))
      ++pos_;
  }
  bool eat(const char c)
  {
    if (peek() == c)
      {
        ++pos_;
        return true;
      }
    return false;
  }
  bool tail_clean()
  {
    skip_ws();
    return pos_ == text_.size();
  }

  bool parse_string(std::string &out)
  {
    if (not eat('"'))
      return false;
    out.clear();
    while (pos_ < text_.size())
      {
        const char c = text_[pos_++];
        if (c == '"')
          return true;
        if (c == '\\')
          {
            if (pos_ >= text_.size())
              return false;
            out.push_back(text_[pos_++]);
          }
        else
          out.push_back(c);
      }
    return false;  // unterminated string.
  }

  bool parse_value(Json_Value &out)
  {
    skip_ws();
    if (peek() == '"')
      {
        std::string s;
        if (not parse_string(s))
          return false;
        out.data = std::move(s);
        return true;
      }
    const std::size_t start = pos_;
    if (peek() == '-' or peek() == '+')
      ++pos_;
    bool any = false;
    while (pos_ < text_.size())
      {
        const char c = text_[pos_];
        if ((c >= '0' and c <= '9') or c == '.' or c == 'e' or c == 'E'
            or c == '+' or c == '-')
          {
            ++pos_;
            any = true;
          }
        else
          break;
      }
    if (not any)
      return false;
    out.data = std::stod(text_.substr(start, pos_ - start));
    return true;
  }
};

std::map<std::string, Json_Value> emit_and_parse(const Bench_Result &result)
{
  std::ostringstream os;
  emit_json(result, os);
  std::string line = os.str();
  EXPECT_FALSE(line.empty());
  EXPECT_EQ(line.back(), '\n');
  Json_Parser parser(line);
  std::map<std::string, Json_Value> obj;
  EXPECT_TRUE(parser.parse_object(obj)) << "not well-formed JSON: " << line;
  return obj;
}

}  // namespace

TEST(CaBenchJson, EmitsWellFormedSchema)
{
  Bench_Result r;
  r.name = "gol_1024";
  r.wall_ns = 3377500712ull;
  r.cells_per_sec = 31045914.995;
  const auto obj = emit_and_parse(r);

  ASSERT_EQ(obj.count("name"), 1u);
  ASSERT_EQ(obj.count("wall_ns"), 1u);
  ASSERT_EQ(obj.count("cells_per_sec"), 1u);
  ASSERT_EQ(obj.count("compiler"), 1u);
  ASSERT_EQ(obj.count("cpu"), 1u);
  EXPECT_EQ(obj.size(), 5u);

  EXPECT_TRUE(obj.at("name").is_string());
  EXPECT_EQ(obj.at("name").str(), "gol_1024");
  EXPECT_TRUE(obj.at("wall_ns").is_number());
  EXPECT_DOUBLE_EQ(obj.at("wall_ns").num(), 3377500712.0);
  EXPECT_TRUE(obj.at("cells_per_sec").is_number());
  EXPECT_NEAR(obj.at("cells_per_sec").num(), 31045914.995, 1.0);
  EXPECT_TRUE(obj.at("compiler").is_string());
  EXPECT_FALSE(obj.at("compiler").str().empty());
  EXPECT_TRUE(obj.at("cpu").is_string());
  EXPECT_FALSE(obj.at("cpu").str().empty());
}

TEST(CaBenchJson, HandlesZeroAndLargeValues)
{
  Bench_Result zero;
  zero.name = "edge_zero";
  zero.wall_ns = 0;
  zero.cells_per_sec = 0.0;
  const auto a = emit_and_parse(zero);
  EXPECT_EQ(a.at("name").str(), "edge_zero");
  EXPECT_DOUBLE_EQ(a.at("wall_ns").num(), 0.0);
  EXPECT_DOUBLE_EQ(a.at("cells_per_sec").num(), 0.0);

  Bench_Result big;
  big.name = "edge_big";
  big.wall_ns = 9000000000000ull;
  big.cells_per_sec = 4.26843e8;
  const auto b = emit_and_parse(big);
  EXPECT_DOUBLE_EQ(b.at("wall_ns").num(), 9000000000000.0);
  EXPECT_GT(b.at("cells_per_sec").num(), 0.0);
}

TEST(CaBenchJson, CpuStringIsJsonSafe)
{
  // The live CPU string must never contain raw quotes, commas or backslashes
  // that would corrupt the single-line JSON record.
  const std::string cpu = Aleph::CA::Bench::cpu_string();
  EXPECT_EQ(cpu.find('"'), std::string::npos);
  EXPECT_EQ(cpu.find(','), std::string::npos);
  EXPECT_EQ(cpu.find('\\'), std::string::npos);
}