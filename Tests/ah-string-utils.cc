//
// Created by lrleon on 23/04/24.
//
# include <gtest/gtest.h>

# include <cstdlib>
# include <limits>
# include <random>
# include <stdexcept>
# include <vector>

# include <ah-string-utils.H>

using namespace std;
using namespace testing;
using namespace Aleph;

TEST(util, concat)
{
  string s1 = "hello";
  string s2 = "world";
  string blank = " ";
  string s3 = "!";
  string s = concat(s1, blank, s2, s3);
  ASSERT_EQ(s, "hello world!");
}

TEST(StringUtils, vector_to_string)
{
  vector<int> v = {1, 2, 3};
  EXPECT_EQ(Aleph::to_string(v), "1, 2, 3");
  vector<int> e;
  EXPECT_EQ(Aleph::to_string(e), "");
}

TEST(StringUtils, array_to_string)
{
  Array<int> a;
  EXPECT_EQ(Aleph::to_string(a), "");
  a.append(1);
  EXPECT_EQ(Aleph::to_string(a), "1");
  a.append(2);
  a.append(3);
  EXPECT_EQ(Aleph::to_string(a), "1, 2, 3");
}

TEST(StringUtils, trim_variants)
{
  {
    string s = "\t  abc  \n";
    EXPECT_EQ(trim(s), "abc");
    EXPECT_EQ(s, "\t  abc  \n");
  }

  {
    string s = "\t  abc  \n";
    EXPECT_EQ(trim_in_place(s), "abc");
    EXPECT_EQ(s, "abc");
  }

  {
    string s = "";
    EXPECT_EQ(trim(s), "");
    EXPECT_EQ(trim_in_place(s), "");
  }
}

TEST(StringUtils, contains_string_view)
{
  EXPECT_TRUE(contains("hello world", "world"));
  EXPECT_FALSE(contains("hello", "xyz"));
  EXPECT_TRUE(contains("", ""));
}

TEST(StringUtils, double_formatting)
{
  EXPECT_EQ(Aleph::to_string(1.5, 2), "1.50");
  EXPECT_EQ(Aleph::to_string(1.0, 0), "1");
  auto s = to_str(1.0 / 3.0);
  EXPECT_FALSE(s.empty());
}

TEST(StringUtils, case_conversion_ascii)
{
  EXPECT_EQ(Aleph::tolower("HeLLo"), "hello");
  EXPECT_EQ(Aleph::toupper("HeLLo"), "HELLO");

  string s = "HeLLo";
  EXPECT_EQ(mutable_tolower(s), "hello");
  EXPECT_EQ(s, "hello");
  EXPECT_EQ(mutable_toupper(s), "HELLO");
  EXPECT_EQ(s, "HELLO");
}

TEST(StringUtils, case_conversion_non_ascii_does_not_crash)
{
  string s;
  s.push_back(static_cast<char>(0xFF));
  s.push_back('A');
  auto lower = Aleph::to_lower(s);
  ASSERT_EQ(lower.size(), 2u);
  EXPECT_EQ(lower[1], 'a');
}

TEST(StringUtils, only_alpha_and_removals)
{
  EXPECT_EQ(only_alpha("A-bC_9"), "abc9");
  EXPECT_EQ(remove_spaces(" a\tb\nc "), "abc");
  EXPECT_EQ(remove_symbols("a-b_c", "-_"), "abc");
  EXPECT_EQ(remove_symbols("", "-_"), "");
}

TEST(StringUtils, join_dynlist)
{
  DynList<int> l;
  EXPECT_EQ(join(l, ","), "");
  l.append(1);
  l.append(2);
  l.append(3);
  EXPECT_EQ(join(l, ","), "1,2,3");
  EXPECT_EQ(join(l, " - "), "1 - 2 - 3");
}

TEST(StringUtils, numeric_parsers)
{
  EXPECT_TRUE(is_long("0"));
  EXPECT_TRUE(is_long("-10"));
  EXPECT_FALSE(is_long(""));
  EXPECT_FALSE(is_long("10x"));

  EXPECT_TRUE(is_size_t("0"));
  EXPECT_TRUE(is_size_t("10"));
  EXPECT_FALSE(is_size_t("-1"));
  EXPECT_FALSE(is_size_t(""));
  EXPECT_FALSE(is_size_t("10x"));

  EXPECT_TRUE(is_double("0"));
  EXPECT_TRUE(is_double("-1.25"));
  EXPECT_TRUE(is_double("1e3"));
  EXPECT_FALSE(is_double(""));
  EXPECT_FALSE(is_double("1.2x"));
  EXPECT_FALSE(is_double("1e309"));

  EXPECT_TRUE(is_float("0"));
  EXPECT_TRUE(is_float("-1.25"));
  EXPECT_FALSE(is_float(""));
  EXPECT_FALSE(is_float("1.2x"));
}

TEST(StringUtils, prefix_and_remove_prefix)
{
  EXPECT_TRUE(is_prefix("foobar", "foo"));
  EXPECT_FALSE(is_prefix("foo", "foobar"));

  string s = "prefix_value";
  EXPECT_EQ(remove_prefix(s, "prefix_"), "value");
  EXPECT_EQ(s, "value");
}

TEST(StringUtils, to_name)
{
  EXPECT_EQ(to_name(""), "");
  EXPECT_EQ(to_name("hello"), "Hello");
  EXPECT_EQ(to_name("Hello"), "Hello");
}

TEST(StringUtils, split_camel_case)
{
  {
    auto parts = split_camel_case("");
    EXPECT_TRUE(parts.is_empty());
  }

  {
    auto parts = split_camel_case("camelCaseString");
    ASSERT_EQ(parts.size(), 3u);
    EXPECT_EQ(parts.nth(0), "camel");
    EXPECT_EQ(parts.nth(1), "Case");
    EXPECT_EQ(parts.nth(2), "String");
  }
}

TEST(StringUtils, split_and_split_string)
{
  {
    auto v = split("a,b,,c", ',');
    ASSERT_EQ(v.size(), 4u);
    EXPECT_EQ(v[0], "a");
    EXPECT_EQ(v[1], "b");
    EXPECT_EQ(v[2], "");
    EXPECT_EQ(v[3], "c");
  }

  {
    auto l = split_to_list("a--b---c", "-");
    ASSERT_EQ(l.size(), 3u);
    EXPECT_EQ(l.nth(0), "a");
    EXPECT_EQ(l.nth(1), "b");
    EXPECT_EQ(l.nth(2), "c");
  }

  {
    auto l = split_to_list("a b-c__d", " _-");
    ASSERT_EQ(l.size(), 4u);
    EXPECT_EQ(l.nth(0), "a");
    EXPECT_EQ(l.nth(1), "b");
    EXPECT_EQ(l.nth(2), "c");
    EXPECT_EQ(l.nth(3), "d");
  }

  {
    auto l = split_to_list("abc", "");
    ASSERT_EQ(l.size(), 1u);
    EXPECT_EQ(l.nth(0), "abc");
  }
}

TEST(StringUtils, pascal_case)
{
  EXPECT_EQ(to_Pascalcase("hello_world"), "HelloWorld");
  EXPECT_EQ(to_Pascalcase("alreadyPascal"), "AlreadyPascal");
}

TEST(StringUtils, split_pos)
{
  string s = "abcd";
  EXPECT_EQ(split_pos(s, 0), (pair<string, string>("", "abcd")));
  EXPECT_EQ(split_pos(s, 2), (pair<string, string>("ab", "cd")));
  EXPECT_EQ(split_pos(s, 4), (pair<string, string>("abcd", "")));
  EXPECT_THROW(split_pos(s, 5), range_error);
}

TEST(StringUtils, split_n)
{
  EXPECT_THROW(split_n("abc", 0), range_error);
  EXPECT_THROW(split_n("abc", 4), range_error);

  auto l = split_n("abcdef", 4);
  ASSERT_EQ(l.size(), 4u);
  EXPECT_EQ(l.nth(0), "a");
  EXPECT_EQ(l.nth(1), "b");
  EXPECT_EQ(l.nth(2), "c");
  EXPECT_EQ(l.nth(3), "def");
}

TEST(StringUtils, complete_rows)
{
  DynList<DynList<int>> m;
  DynList<int> r1; r1.append(1); r1.append(2);
  DynList<int> r2; r2.append(3);
  m.append(r1);
  m.append(r2);

  auto out = complete_rows(m);
  ASSERT_EQ(out.size(), 2u);
  ASSERT_EQ(out.nth(0).size(), 2u);
  ASSERT_EQ(out.nth(1).size(), 2u);
  EXPECT_EQ(out.nth(1).nth(0), 3);
  EXPECT_EQ(out.nth(1).nth(1), 0);
}

TEST(StringUtils, format_string_no_underflow)
{
  DynList<DynList<string>> mat;
  DynList<string> r1; r1.append("abcd"); r1.append("x");
  DynList<string> r2; r2.append("ab"); r2.append("xyz");
  mat.append(r1);
  mat.append(r2);

  DynList<size_t> lens; lens.append(2); lens.append(1);
  auto formatted = Aleph::format_string(lens, mat);
  ASSERT_EQ(formatted.size(), 2u);
  ASSERT_EQ(formatted.nth(0).size(), 2u);
}

TEST(StringUtils, format_string_csv)
{
  DynList<DynList<string>> mat;
  DynList<string> r1; r1.append("a"); r1.append("b"); r1.append("c");
  mat.append(r1);
  auto csv = format_string_csv(mat);
  ASSERT_EQ(csv.size(), 1u);
  EXPECT_EQ(csv.nth(0).nth(0), "a,");
  EXPECT_EQ(csv.nth(0).nth(1), "b,");
  EXPECT_EQ(csv.nth(0).nth(2), "c");
}

TEST(StringUtils, text_wrapping_and_shift)
{
  const string text = "one two three four five";
  auto j = justify_text(text, 10, 2);
  EXPECT_TRUE(contains(j, "  "));

  auto a = align_text_to_left(text, 10, 1);
  EXPECT_TRUE(contains(a, " one"));

  auto shifted = shift_lines_to_left("a\nb", 3);
  EXPECT_EQ(shifted, "   a\n   b");
}

TEST(StringUtils, build_pars_list_variadic)
{
  EXPECT_EQ(::Aleph::build_pars_list(1), "1");
  EXPECT_EQ(::Aleph::build_pars_list(1, 2, 3), "1, 2, 3");
}

TEST(StringUtils, build_pars_list_container)
{
  DynList<int> l;
  EXPECT_EQ(::Aleph::build_pars_list(l), "");
  l.append(1);
  l.append(2);
  l.append(3);
  EXPECT_EQ(::Aleph::build_pars_list(l), "1,2,3");
  EXPECT_EQ(::Aleph::build_pars_list(l, " | "), "1 | 2 | 3");
}

TEST(StringUtils, fill_string)
{
  string s = "secret";
  fill_string(s, 'x');
  EXPECT_EQ(s, "xxxxxx");

  string e;
  fill_string(e, 'x');
  EXPECT_TRUE(e.empty());
}

TEST(StringUtils, split_to_array)
{
  auto a = split_to_array("a b-c__d", " _-");
  ASSERT_EQ(a.size(), 4u);
  EXPECT_EQ(a[0], "a");
  EXPECT_EQ(a[1], "b");
  EXPECT_EQ(a[2], "c");
  EXPECT_EQ(a[3], "d");
}

TEST(StringUtils, format_string_computed_lens)
{
  DynList<DynList<string>> mat;
  DynList<string> r1; r1.append("abcd"); r1.append("x");
  DynList<string> r2; r2.append("ab"); r2.append("xyz");
  mat.append(r1);
  mat.append(r2);

  auto formatted = Aleph::format_string(mat);
  ASSERT_EQ(formatted.size(), 2u);
  ASSERT_EQ(formatted.nth(0).size(), 2u);
  ASSERT_EQ(formatted.nth(1).size(), 2u);
}

TEST(StringUtils, to_string_matrix_and_lines)
{
  DynList<DynList<string>> mat;
  DynList<string> r1; r1.append("a"); r1.append("b");
  DynList<string> r2; r2.append("c"); r2.append("d");
  mat.append(r1);
  mat.append(r2);
  auto s = Aleph::to_string(mat);
  EXPECT_TRUE(contains(s, "a"));
  EXPECT_TRUE(contains(s, "d"));

  DynList<string> lines;
  lines.append("x");
  lines.append("y");
  EXPECT_EQ(Aleph::to_string(lines), "x\ny");
}

TEST(StringUtils, split_text_into_words_and_lines)
{
  auto w = split_text_into_words("  a\t b\n c  ");
  ASSERT_EQ(w.size(), 3u);
  EXPECT_EQ(w.nth(0), "a");
  EXPECT_EQ(w.nth(1), "b");
  EXPECT_EQ(w.nth(2), "c");

  auto l = split_text_into_lines("a\nb\n");
  ASSERT_EQ(l.size(), 2u);
  EXPECT_EQ(l.nth(0), "a");
  EXPECT_EQ(l.nth(1), "b");
}

TEST(StringUtils, justify_and_align_except_first)
{
  const string text = "one two three four five";
  auto j = justify_line_except_first(text, 10, 4);
  EXPECT_TRUE(contains(j, "one"));
  EXPECT_TRUE(contains(j, "\n"));

  auto a = align_text_to_left_except_first(text, 10, 3);
  EXPECT_TRUE(contains(a, "one"));
  EXPECT_TRUE(contains(a, "\n"));
}

TEST(StringUtils, numeric_boundaries)
{
  const auto max_st = std::numeric_limits<size_t>::max();
  EXPECT_TRUE(is_size_t(::std::to_string(max_st)));
  EXPECT_FALSE(is_size_t(::std::to_string(max_st) + "0"));

  const auto max_l = std::numeric_limits<long>::max();
  const auto min_l = std::numeric_limits<long>::min();
  EXPECT_TRUE(is_long(::std::to_string(max_l)));
  EXPECT_TRUE(is_long(::std::to_string(min_l)));
  EXPECT_FALSE(is_long(::std::to_string(max_l) + "0"));
}

static int stress_multiplier()
{
  const char *v = std::getenv("ALEPH_STRESS");
  if (v == nullptr or *v == '\0')
    return 1;

  char *end = nullptr;
  const long m = std::strtol(v, &end, 10);
  if (end == v or *end != '\0')
    return 1;

  if (m < 1)
    return 1;

  if (m > 50)
    return 50;

  return int(m);
}

static string random_string(std::mt19937 & rng, size_t len)
{
  std::uniform_int_distribution<int> byte_dist(0, 255);
  string s;
  s.reserve(len);
  for (size_t i = 0; i < len; ++i)
    s.push_back(static_cast<char>(byte_dist(rng)));
  return s;
}

static string random_ascii_token(std::mt19937 & rng, size_t len)
{
  static constexpr char alphabet[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::uniform_int_distribution<int> pick(0, int(sizeof(alphabet) - 2));
  string s;
  s.reserve(len);
  for (size_t i = 0; i < len; ++i)
    s.push_back(alphabet[pick(rng)]);
  return s;
}

TEST(StringUtilsStress, split_pos_recomposes)
{
  std::mt19937 rng(12345);
  for (int iter = 0; iter < 2000*stress_multiplier(); ++iter)
    {
      const auto s = random_ascii_token(rng, size_t(iter % 64));
      for (size_t pos = 0; pos <= s.size(); ++pos)
        {
          auto p = split_pos(s, pos);
          EXPECT_EQ(p.first + p.second, s);
          EXPECT_EQ(p.first.size(), pos);
        }
    }
}

TEST(StringUtilsStress, split_n_recomposes)
{
  std::mt19937 rng(54321);
  for (int iter = 0; iter < 1500*stress_multiplier(); ++iter)
    {
      const auto s = random_ascii_token(rng, 1 + size_t(iter % 128));
      const size_t n = 1 + (size_t(iter) % std::min<size_t>(16, s.size()));
      auto parts = split_n(s, n);
      ASSERT_EQ(parts.size(), n);
      string recomposed;
      parts.for_each([&](const string &x) { recomposed += x; });
      EXPECT_EQ(recomposed, s);

      const size_t base = s.size() / n;
      for (size_t i = 0; i + 1 < n; ++i)
        EXPECT_EQ(parts.nth(i).size(), base);
    }
}

TEST(StringUtilsStress, split_string_tokens_have_no_delims)
{
  std::mt19937 rng(999);
  static const string delims = " _-";
  for (int iter = 0; iter < 2000*stress_multiplier(); ++iter)
    {
      string s;
      const size_t tokens = 1 + (size_t(iter) % 12);
      for (size_t i = 0; i < tokens; ++i)
        {
          if (i)
            s.push_back(delims[size_t(iter + int(i)) % delims.size()]);
          s += random_ascii_token(rng, 1 + (size_t(iter + int(i)) % 10));
        }

      auto out = split_to_list(s, delims);
      ASSERT_FALSE(out.is_empty());
      out.for_each([&](const string &t)
      {
        EXPECT_FALSE(t.empty());
        for (char c : delims)
          EXPECT_EQ(t.find(c), string::npos);
      });
    }
}

TEST(StringUtilsStress, case_conversion_no_crash_and_preserves_length)
{
  std::mt19937 rng(2024);
  for (int iter = 0; iter < 3000*stress_multiplier(); ++iter)
    {
      auto s = random_string(rng, size_t(iter % 128));
      auto lo = Aleph::to_lower(s);
      auto up = Aleph::to_upper(s);
      EXPECT_EQ(lo.size(), s.size());
      EXPECT_EQ(up.size(), s.size());
    }
}

TEST(StringUtilsStress, build_pars_list_round_trip_for_ints)
{
  std::mt19937 rng(77);
  std::uniform_int_distribution<int> dist(-100000, 100000);

  for (int iter = 0; iter < 2000*stress_multiplier(); ++iter)
    {
      const int a = dist(rng);
      const int b = dist(rng);
      const int c = dist(rng);

      const auto got = ::Aleph::build_pars_list(a, b, c);
      const auto expected = ::std::to_string(a) + ", " + ::std::to_string(b) + ", " + ::std::to_string(c);
      EXPECT_EQ(got, expected);
    }
}