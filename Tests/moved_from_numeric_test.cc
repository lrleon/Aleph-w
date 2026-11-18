
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file moved_from_numeric_test.cc
 *  @brief The moved-from tests (moved_from.H) of the transforms and the
 *         filters, the string automata, the geometric indexes and the
 *         search collector.
 */

#include "moved_from.H"

#include <Aho_Corasick.H>
#include <Suffix_Structures.H>
#include <fft.H>
#include <geom_algorithms.H>
#include <ntt.H>
#include <state_search_common.H>

#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>

using namespace Aleph;

using F = FFT<double>;
using Complex = F::Complex;
using N = NTT<>;
using Collector = SearchSolutionCollector<int>;

namespace
{
  // n samples in (0, 1]: no kernel, window or signal made with it is zero
  Array<double> ramp(const size_t n)
  {
    Array<double> a;
    for (size_t i = 0; i < n; ++i)
      a.append(static_cast<double>(1 + i % 17) / 17.0);
    return a;
  }

  // A text of n bytes over the letters a to e
  std::string text(const size_t n)
  {
    std::string s;
    for (size_t i = 0; i < n; ++i)
      s += static_cast<char>('a' + (i * 7) % 5);
    return s;
  }

  template <typename T>
  void append_all(Array<T> & out, const Array<T> & items)
  {
    for (const T & item : items)
      out.append(item);
  }

  // The linear convolution, by its definition
  Array<double> direct_convolution(const Array<double> & signal, const Array<double> & kernel)
  {
    Array<double> out(signal.size() + kernel.size() - 1, 0.0);
    for (size_t i = 0; i < signal.size(); ++i)
      for (size_t j = 0; j < kernel.size(); ++j)
        out(i + j) += signal[i] * kernel[j];
    return out;
  }

  // The output from rest of the filter b / a, with a[0] == 1
  Array<double> direct_iir(const Array<double> & x, const Array<double> & b,
                           const Array<double> & a)
  {
    Array<double> y(x.size(), 0.0);
    for (size_t n = 0; n < x.size(); ++n)
      {
        double acc = 0;
        for (size_t k = 0; k < b.size() and k <= n; ++k)
          acc += b[k] * x[n - k];
        for (size_t k = 1; k < a.size() and k <= n; ++k)
          acc -= a[k] * y[n - k];
        y(n) = acc;
      }
    return y;
  }

  void expect_near(const Array<double> & got, const Array<double> & expected)
  {
    ASSERT_EQ(got.size(), expected.size());
    for (size_t i = 0; i < got.size(); ++i)
      EXPECT_NEAR(got[i], expected[i], 1e-9);
  }
}

template <> struct Moved_From_Case<F::Plan>
{
  // Odd sizes, for the Bluestein (101, 1001) and the mixed-radix (3) plans;
  // the convolvers below hold power-of-two plans
  static F::Plan make(const size_t n) { return F::Plan(n | 1); }
  static bool is_empty(const F::Plan & p) { return p.size() == 0; }
  static void use(F::Plan & p)
  {
    Array<Complex> a = Array<Complex>::create(p.size());
    for (size_t i = 0; i < a.size(); ++i)
      a(i) = Complex(static_cast<double>(i % 7), 1.0);
    const Array<Complex> back = p.inverse_transform(p.transformed(a));
    ASSERT_EQ(back.size(), a.size());
    for (size_t i = 0; i < a.size(); ++i)
      EXPECT_LT(std::abs(back[i] - a[i]), 1e-9);
  }
};

// The convolvers are left empty when they are moved from: they reject every
// block and signal, and reset() and flush() have nothing to do
template <> struct Moved_From_Case<F::OverlapAdd>
{
  static F::OverlapAdd make(const size_t n)
  {
    F::OverlapAdd c(ramp(n / 10 + 1), n);
    (void) c.process_block(ramp(n));  // leaves a pending tail
    return c;
  }
  static bool is_empty(const F::OverlapAdd & c)
  {
    return c.kernel().is_empty() and c.block_size() == 0 and c.fft_size() == 0
           and c.overlap_size() == 0;
  }
  static void use(F::OverlapAdd & c)
  {
    if (c.kernel().is_empty())
      {
        ThreadPool pool(1);
        EXPECT_THROW((void) c.process_block(ramp(8)), std::runtime_error);
        EXPECT_THROW((void) c.pprocess_block(pool, ramp(8)), std::runtime_error);
        EXPECT_THROW((void) c.convolve(ramp(8)), std::runtime_error);
        EXPECT_THROW((void) c.pconvolve(pool, ramp(8)), std::runtime_error);
        c.reset();
        EXPECT_TRUE(c.flush().is_empty());
        return;
      }

    const Array<double> signal = ramp(3 * c.block_size() + 1);
    const Array<double> expected = direct_convolution(signal, c.kernel());
    expect_near(c.convolve(signal), expected);
    c.reset();
    Array<double> streamed = c.process_block(signal);
    append_all(streamed, c.flush());
    expect_near(streamed, expected);
  }
};

template <> struct Moved_From_Case<F::OverlapAddBank>
{
  static Array<Array<double>> signals(const size_t channels, const size_t n)
  {
    Array<Array<double>> s;
    for (size_t c = 0; c < channels; ++c)
      s.append(ramp(n + c));
    return s;
  }
  static F::OverlapAddBank make(const size_t n)
  {
    F::OverlapAddBank b(n < 10 ? 1 : 3, ramp(n / 10 + 1), n);
    (void) b.process_block(signals(b.num_channels(), n));  // leaves pending tails
    return b;
  }
  static bool is_empty(const F::OverlapAddBank & b)
  {
    return b.num_channels() == 0 and b.kernel().is_empty() and b.block_size() == 0
           and b.overlap_size() == 0;
  }
  static void use(F::OverlapAddBank & b)
  {
    if (b.kernel().is_empty())
      {
        ThreadPool pool(1);
        const Array<Array<double>> none;  // as many channels as the bank has
        EXPECT_THROW((void) b.process_block(none), std::runtime_error);
        EXPECT_THROW((void) b.pprocess_block(pool, none), std::runtime_error);
        EXPECT_THROW((void) b.convolve(none), std::runtime_error);
        EXPECT_THROW((void) b.pconvolve(pool, none), std::runtime_error);
        b.reset();
        EXPECT_TRUE(b.flush().is_empty());
        return;
      }

    const Array<Array<double>> in = signals(b.num_channels(), 2 * b.block_size() + 1);
    const Array<Array<double>> out = b.convolve(in);
    ASSERT_EQ(out.size(), in.size());
    b.reset();
    Array<Array<double>> streamed = b.process_block(in);
    const Array<Array<double>> tails = b.flush();
    ASSERT_EQ(streamed.size(), in.size());
    ASSERT_EQ(tails.size(), in.size());
    for (size_t c = 0; c < in.size(); ++c)
      {
        const Array<double> expected = direct_convolution(in[c], b.kernel());
        expect_near(out[c], expected);
        append_all(streamed(c), tails[c]);
        expect_near(streamed[c], expected);
      }
  }
};

template <> struct Moved_From_Case<F::OverlapSave>
{
  static F::OverlapSave make(const size_t n)
  {
    F::OverlapSave c(ramp(n / 10 + 1), n);
    (void) c.process_block(ramp(n + n / 2));  // leaves samples pending
    return c;
  }
  static bool is_empty(const F::OverlapSave & c)
  {
    return c.kernel().is_empty() and c.block_size() == 0 and c.fft_size() == 0
           and c.overlap_size() == 0;
  }
  static void use(F::OverlapSave & c)
  {
    if (c.kernel().is_empty())
      {
        EXPECT_THROW((void) c.process_block(ramp(8)), std::runtime_error);
        EXPECT_THROW((void) c.convolve(ramp(8)), std::runtime_error);
        c.reset();
        EXPECT_TRUE(c.flush().is_empty());
        return;
      }

    const Array<double> signal = ramp(3 * c.block_size() + 1);
    const Array<double> expected = direct_convolution(signal, c.kernel());
    expect_near(c.convolve(signal), expected);
    Array<double> streamed = c.process_block(signal);
    append_all(streamed, c.flush());
    expect_near(streamed, expected);
  }
};

template <> struct Moved_From_Case<F::PartitionedConvolver>
{
  static F::PartitionedConvolver make(const size_t n)
  {
    F::PartitionedConvolver c(ramp(n + 1), n < 10 ? 2 : 32);
    (void) c.process_block(ramp(n / 2 + 1));  // leaves a state
    return c;
  }
  static bool is_empty(const F::PartitionedConvolver & c)
  {
    return c.kernel().is_empty() and c.partition_size() == 0 and c.fft_size() == 0;
  }
  static void use(F::PartitionedConvolver & c)
  {
    if (c.kernel().is_empty())
      {
        EXPECT_THROW((void) c.process_block(ramp(8)), std::runtime_error);
        EXPECT_THROW((void) c.convolve(ramp(8)), std::runtime_error);
        c.reset();
        EXPECT_TRUE(c.flush().is_empty());
        return;
      }

    const Array<double> signal = ramp(3 * c.partition_size() + 1);
    const Array<double> expected = direct_convolution(signal, c.kernel());
    expect_near(c.convolve(signal), expected);
    Array<double> streamed = c.process_block(signal);
    append_all(streamed, c.flush());
    expect_near(streamed, expected);
  }
};

// The processors and the filters are left unconfigured, as built by their
// default constructors: then every operation says so. The destination of a
// move goes on as its source would have: the fingerprint is what a copy
// outputs for the next block, after the samples or the state that make()
// leaves pending, which use() then resets
template <> struct Moved_From_Case<F::STFTProcessor>
{
  static F::STFTProcessor make(const size_t n)
  {
    const size_t frame = n < 10 ? 2 : n / 4;
    F::STFTOptions o;
    o.hop_size = frame / 2;
    F::STFTProcessor s(ramp(frame), o);
    (void) s.process_block(ramp(n));  // leaves samples pending
    return s;
  }
  static bool is_empty(const F::STFTProcessor & s)
  {
    return not s.configured() and s.frame_size() == 0 and s.fft_size() == 0;
  }
  static Array<Array<Complex>> fingerprint(const F::STFTProcessor & s)
  {
    if (not s.configured())
      return {};
    F::STFTProcessor copy(s);
    return copy.process_block(ramp(3 * s.frame_size()));
  }
  static void use(F::STFTProcessor & s)
  {
    if (not s.configured())
      {
        EXPECT_THROW((void) s.process_block(ramp(8)), std::runtime_error);
        EXPECT_THROW(s.reset(), std::runtime_error);
        return;
      }

    F::STFTOptions o;
    o.hop_size = s.hop_size();
    o.fft_size = s.fft_size();
    o.centered = s.centered();
    o.pad_end = s.pad_end();
    const Array<double> signal = ramp(5 * s.frame_size());
    const Array<Array<Complex>> expected = F::stft(signal, s.window(), o);
    s.reset();
    Array<Array<Complex>> frames = s.process_block(signal);
    append_all(frames, s.flush());
    ASSERT_EQ(frames.size(), expected.size());
    for (size_t f = 0; f < frames.size(); ++f)
      {
        ASSERT_EQ(frames[f].size(), expected[f].size());
        for (size_t k = 0; k < frames[f].size(); ++k)
          EXPECT_LT(std::abs(frames[f][k] - expected[f][k]), 1e-9);
      }
  }
};

template <> struct Moved_From_Case<F::ISTFTProcessor>
{
  static F::ISTFTOptions options(const size_t hop)
  {
    F::ISTFTOptions o;
    o.hop_size = hop;
    return o;
  }
  // The frames of n samples; ramp(frame) is the window of every processor
  static Array<Array<Complex>> spectrogram(const size_t frame, const size_t hop,
                                           const size_t fft, const size_t n)
  {
    F::STFTOptions o;
    o.hop_size = hop;
    o.fft_size = fft;
    return F::stft(ramp(n), ramp(frame), o);
  }
  static F::ISTFTProcessor make(const size_t n)
  {
    const size_t frame = n < 10 ? 2 : n / 4;
    const size_t fft = std::bit_ceil(frame);
    F::ISTFTProcessor p(fft, ramp(frame), options(frame / 2));
    (void) p.process_block(spectrogram(frame, frame / 2, fft, n));  // leaves samples pending
    return p;
  }
  static bool is_empty(const F::ISTFTProcessor & p)
  {
    return not p.configured() and p.frame_size() == 0 and p.fft_size() == 0;
  }
  static Array<double> fingerprint(const F::ISTFTProcessor & p)
  {
    if (not p.configured())
      return {};
    F::ISTFTProcessor copy(p);
    return copy.process_block(
      spectrogram(p.frame_size(), p.hop_size(), p.fft_size(), 3 * p.frame_size()));
  }
  static void use(F::ISTFTProcessor & p)
  {
    if (not p.configured())
      {
        EXPECT_THROW((void) p.process_frame(Array<Complex>({Complex(1.0, 0.0)})),
                     std::runtime_error);
        EXPECT_THROW((void) p.flush(), std::runtime_error);
        return;
      }

    const Array<Array<Complex>> frames =
      spectrogram(p.frame_size(), p.hop_size(), p.fft_size(), 5 * p.frame_size());
    const Array<double> expected = F::istft(frames, ramp(p.frame_size()), options(p.hop_size()));
    p.reset();
    Array<double> streamed = p.process_block(frames);
    append_all(streamed, p.flush());
    expect_near(streamed, expected);
  }
};

template <> struct Moved_From_Case<F::LFilter>
{
  static F::LFilter make(const size_t n)
  {
    const bool small = n < 10;
    F::LFilter f(small ? Array<double>{0.5, 0.5} : Array<double>{0.2, 0.3, 0.1},
                 small ? Array<double>{1.0, -0.25} : Array<double>{1.0, -0.4, 0.1});
    (void) f.filter(ramp(n));  // leaves a state
    return f;
  }
  static bool is_empty(const F::LFilter & f)
  {
    return not f.configured() and f.order() == 0 and f.state().is_empty();
  }
  static Array<double> fingerprint(const F::LFilter & f)
  {
    if (not f.configured())
      return {};
    F::LFilter copy(f);
    return copy.filter(ramp(20));
  }
  static void use(F::LFilter & f)
  {
    if (not f.configured())
      {
        EXPECT_THROW((void) f.filter(ramp(8)), std::runtime_error);
        EXPECT_THROW(f.reset(), std::runtime_error);
        return;
      }

    f.reset();
    const Array<double> signal = ramp(50);
    expect_near(f.filter(signal),
                direct_iir(signal, f.coefficients().numerator, f.coefficients().denominator));
    EXPECT_EQ(f.state().size(), f.order());
    f.set_state(Array<double>(f.order(), 0.0));
  }
};

template <> struct Moved_From_Case<F::SOSFilter>
{
  // The section of every cascade
  static F::BiquadSection section() { return {0.2, 0.3, 0.1, 1.0, -0.4, 0.1}; }
  static F::SOSFilter make(const size_t n)
  {
    F::SOSFilter f(Array<F::BiquadSection>(n < 10 ? 1 : 3, section()));
    (void) f.filter(ramp(n));  // leaves a state
    return f;
  }
  static bool is_empty(const F::SOSFilter & f)
  {
    return not f.configured() and f.num_sections() == 0;
  }
  static Array<double> fingerprint(const F::SOSFilter & f)
  {
    if (not f.configured())
      return {};
    F::SOSFilter copy(f);
    return copy.filter(ramp(20));
  }
  static void use(F::SOSFilter & f)
  {
    if (not f.configured())
      {
        EXPECT_THROW((void) f.filter(ramp(8)), std::runtime_error);
        EXPECT_THROW(f.reset(), std::runtime_error);
        return;
      }

    f.reset();
    const Array<double> signal = ramp(50);
    Array<double> expected = signal;
    for (size_t i = 0; i < f.num_sections(); ++i)
      expected = direct_iir(expected, section().numerator(), section().denominator());
    expect_near(f.filter(signal), expected);
    for (size_t i = 0; i < f.num_sections(); ++i)
      EXPECT_EQ(f.state(i).size(), 2u);
  }
};

template <> struct Moved_From_Case<N::Plan>
{
  // 7 takes the Bluestein path (14 divides MOD - 1), the powers of two the
  // radix-2 one
  static N::Plan make(const size_t n) { return N::Plan(n < 10 ? 7 : std::bit_ceil(n)); }
  static bool is_empty(const N::Plan & p) { return p.size() == 0; }
  static void use(N::Plan & p)
  {
    Array<uint64_t> a = Array<uint64_t>::create(p.size());
    for (size_t i = 0; i < a.size(); ++i)
      a(i) = i % 13;
    const Array<uint64_t> back = p.transformed(p.transformed(a), true);
    ASSERT_EQ(back.size(), a.size());
    for (size_t i = 0; i < a.size(); ++i)
      EXPECT_EQ(back[i], a[i]);
    if (p.size() < 3)
      return;

    const Array<uint64_t> product = p.multiply(Array<uint64_t>{1, 2}, Array<uint64_t>{3, 4});
    ASSERT_EQ(product.size(), 3u);
    EXPECT_EQ(product[0], 3u);
    EXPECT_EQ(product[1], 10u);
    EXPECT_EQ(product[2], 8u);
  }
};

// The automata are left as built by their default constructors: with a root
template <> struct Moved_From_Case<Aho_Corasick>
{
  static Aho_Corasick make(const size_t n)
  {
    Aho_Corasick ac;
    for (size_t i = 0; i < n; ++i)
      (void) ac.add_pattern("p" + std::to_string(i));
    ac.build();
    return ac;
  }
  static bool is_empty(const Aho_Corasick & ac)
  {
    return ac.pattern_count() == 0 and not ac.is_built();
  }
  static void use(Aho_Corasick & ac)
  {
    for (size_t i = 0; i < ac.pattern_count(); ++i)
      EXPECT_FALSE(ac.pattern(i).empty());
    const size_t id = ac.add_pattern("zq");
    ac.build();
    EXPECT_TRUE(ac.contains_any("xxzqyy"));
    bool found = false;
    for (const Aho_Corasick::Match & m : ac.search("xxzqyy"))
      found = found or (m.pattern_id == id and m.position == 2);
    EXPECT_TRUE(found);
    ac.clear();
    EXPECT_EQ(ac.pattern_count(), 0u);
  }
};

template <> struct Moved_From_Case<Suffix_Automaton>
{
  static Suffix_Automaton make(const size_t n)
  {
    Suffix_Automaton sam;
    sam.build(text(n));
    return sam;
  }
  static bool is_empty(const Suffix_Automaton & sam)
  {
    return sam.state_count() == 1 and sam.distinct_substring_count() == 0;
  }
  static void use(Suffix_Automaton & sam)
  {
    for (size_t i = 1; i < sam.state_count(); ++i)
      EXPECT_GT(sam.states()[i].len, 0u);
    sam.extend('z');
    EXPECT_TRUE(sam.contains("z"));
    sam.build("abcab");
    EXPECT_EQ(sam.distinct_substring_count(), 12u);
    EXPECT_EQ(sam.longest_common_substring("xcabx"), "cab");
    sam.clear();
    EXPECT_EQ(sam.state_count(), 1u);
  }
};

template <> struct Moved_From_Case<Naive_Suffix_Tree>
{
  static Naive_Suffix_Tree make(const size_t n) { return Naive_Suffix_Tree(text(n)); }
  static bool is_empty(const Naive_Suffix_Tree & t)
  {
    return t.text_size() == 0 and t.find_all("").size() == 1;
  }
  static void use(Naive_Suffix_Tree & t)
  {
    EXPECT_EQ(t.find_all("").size(), t.text_size() + 1);
    EXPECT_FALSE(t.contains("zz"));
    for (const Naive_Suffix_Tree::Node & node : t.nodes())
      for (const size_t child : node.children)
        EXPECT_LT(child, t.node_count());
    t.build("banana");
    EXPECT_TRUE(t.contains("nan"));
    const Array<size_t> positions = t.find_all("ana");
    ASSERT_EQ(positions.size(), 2u);
    EXPECT_EQ(positions[0], 1u);
    EXPECT_EQ(positions[1], 3u);
  }
};

template <> struct Moved_From_Case<RangeTree2D>
{
  static RangeTree2D make(const size_t n)
  {
    DynList<Point> points;
    for (size_t i = 0; i < n; ++i)
      points.append(Point(i, (i * 37) % n));
    RangeTree2D t;
    t.build(points);
    return t;
  }
  static bool is_empty(const RangeTree2D & t) { return t.is_empty() and t.size() == 0; }
  static void use(RangeTree2D & t)
  {
    EXPECT_EQ(t.query(-1, 1 << 20, -1, 1 << 20).size(), t.size());
    EXPECT_EQ(t.debug_snapshot().x_sorted_points.size(), t.size());
    DynList<Point> points;
    points.append(Point(1, 2));
    points.append(Point(3, 4));
    t.build(points);
    EXPECT_EQ(t.size(), 2u);
    EXPECT_EQ(t.query(0, 2, 0, 3).size(), 1u);
  }
};

template <> struct Moved_From_Case<AABBTree>
{
  static AABBTree make(const size_t n)
  {
    Array<AABBTree::Entry> entries;
    for (size_t i = 0; i < n; ++i)
      entries.append({Rectangle(i, i, i + 2, i + 2), i});
    AABBTree t;
    t.build(entries);
    return t;
  }
  static bool is_empty(const AABBTree & t)
  {
    return t.is_empty() and t.debug_snapshot().nodes.is_empty();
  }
  static void use(AABBTree & t)
  {
    EXPECT_EQ(t.query(Rectangle(-1, -1, 1 << 20, 1 << 20)).size(), t.size());
    EXPECT_EQ(t.query_point(Point(1, 1)).is_empty(), t.is_empty());
    if (t.is_empty())  // the braces keep GCC 11 and 13 from -Wdangling-else
      {
        EXPECT_THROW((void) t.root_bbox(), std::domain_error);
      }
    else
      {
        EXPECT_TRUE(t.root_bbox().get_xmin() == 0);
      }
    Array<AABBTree::Entry> one;
    one.append({Rectangle(0, 0, 2, 2), 7});
    t.build(one);
    const Array<size_t> hits = t.query_point(Point(1, 1));
    ASSERT_EQ(hits.size(), 1u);
    EXPECT_EQ(hits[0], 7u);
  }
};

template <> struct Moved_From_Case<Collector>
{
  static Collector make(const size_t n)
  {
    Collector c(n + 5);
    for (size_t i = 0; i < n; ++i)
      (void) c(static_cast<int>(i));
    return c;
  }
  static bool is_empty(const Collector & c)
  {
    return c.is_empty() and c.size() == 0 and c.solutions().is_empty();
  }
  static void use(Collector & c)
  {
    EXPECT_EQ(c.size(), c.solutions().size());
    const size_t before = c.size();
    (void) c(42);
    EXPECT_EQ(c.size(), before + 1);
    EXPECT_EQ(c.solutions().get_last(), 42);
    c.clear();
    EXPECT_TRUE(c.is_empty());
    EXPECT_TRUE(c(1));  // every limit here is above 1
    c.clear();
  }
};

using Numeric = ::testing::Types<F::Plan, F::OverlapAdd, F::OverlapAddBank, F::OverlapSave,
                                 F::PartitionedConvolver, F::STFTProcessor, F::ISTFTProcessor,
                                 F::LFilter, F::SOSFilter, N::Plan, Aho_Corasick,
                                 Suffix_Automaton, Naive_Suffix_Tree, RangeTree2D, AABBTree,
                                 Collector>;
INSTANTIATE_TYPED_TEST_SUITE_P(Numeric, MovedFrom, Numeric);
