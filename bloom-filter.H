/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef BLOOM_FILTER_H
# define BLOOM_FILTER_H

# include <memory>
# include <gsl/gsl_rng.h>
# include <cmath>
# include <iostream>
# include <ahFunctional.H>
# include <bitArray.H>
# include <hash-fct.H>
# include <tpl_dynSetHash.H>

namespace Aleph 
{

template <typename T>
class Bloom_Filter
{
public:

  using Hash_Fct = size_t (*)(const T &, unsigned long seed);

private:

  BitArray bits;

  mutable Hash_Fct hash_fct        = nullptr;  
  mutable unsigned long num_hash   = 0;
  unsigned long * seeds            = nullptr;
  unsigned long * fst_seed         = nullptr;
  unsigned long * last_seed        = nullptr;
  size_t num_ins                   = 0;

public:

  /**

   */
  static tuple<size_t, size_t> estimate(const size_t n, const double p)
  {
    assert(p > 0 and p < 1);
    const double log2 = std::log(2);
    const size_t m = ceil(-(n*std::log(p)) / (log2*log2));
    const size_t k = ceil((1.0*m/n) * log2);
    return make_tuple(m, k);
  }

  size_t get_m() const noexcept { return bits.size(); }

  size_t get_k() const noexcept { return num_hash; }

  size_t get_n() const noexcept { return num_ins; }

  size_t size() const noexcept { return get_n(); }

  size_t capacity() const noexcept { return get_m(); }

  Bloom_Filter(size_t dim, size_t __num_hash, 
	       Hash_Fct __hash_fct = dft_hash_fct,
	       unsigned long seed = time(0))
    : bits(dim), hash_fct(__hash_fct), num_hash(__num_hash),
      seeds(new unsigned long [num_hash]), 
      fst_seed(&seeds[0]), last_seed(&seeds[num_hash])
  {
    bits.reserve(dim);
    unique_ptr<gsl_rng, void(*)(gsl_rng*)>
      r(gsl_rng_alloc(gsl_rng_mt19937), gsl_rng_free);
    if (r.get() == nullptr)
      throw std::bad_alloc();
    gsl_rng_set(r.get(), seed % gsl_rng_max(r.get()));

    for (auto ptr = fst_seed; ptr < last_seed; ++ptr)
      *ptr = gsl_rng_get(r.get());
  }

  Bloom_Filter(size_t n, double p,
	       unsigned long seed = time(0),
	       Hash_Fct __hash_fct = dft_hash_fct)
    : Bloom_Filter(get<0>(estimate(n, p)), get<1>(estimate(n, p)),
		   __hash_fct, seed)
  {
    // empty
  }

  void swap(Bloom_Filter & f) noexcept
  {
    bits.swap(f.bits);
    std::swap(hash_fct, f.hash_fct);
    std::swap(num_hash, f.num_hash);
    std::swap(seeds, f.seeds);
    std::swap(fst_seed, f.fst_seed);
    std::swap(last_seed, f.last_seed);
    std::swap(num_ins, f.num_ins);
  }

  Bloom_Filter(const Bloom_Filter & f)
    : hash_fct(f.hash_fct), num_hash(f.num_hash), 
      seeds(new unsigned long [num_hash]), 
      fst_seed(&seeds[0]), last_seed(&seeds[num_hash]),
      num_ins(f.num_ins)
  {
    bits = f.bits;
    for (auto ptr = fst_seed, seed = f.fst_seed; ptr < last_seed; ++ptr)
      *ptr = *seed++;
  }  

  Bloom_Filter(Bloom_Filter && f)
  {
    swap(f);
  }

  auto & operator = (const Bloom_Filter & f)
  {
    if (this == &f)
      return this;

    if (num_hash != f.num_hash)
      {
	unsigned long * new_seeds = new unsigned long[f.num_hash];
	delete [] seeds;
	seeds = new_seeds;
	num_hash = f.num_hash;
	fst_seed = &seeds[0];
	last_seed = &seeds[num_hash];
      }
    
    bits.set_size(f.size());
    bits = f.bits;
    hash_fct = f.hash_fct;
    for (auto ptr = fst_seed, seed = f.fst_seed; ptr < last_seed; ++ptr)
      *ptr = *seed++;

    num_ins = f.num_ins;

    return *this;
  }

  auto & operator = (Bloom_Filter && f)
  {
    swap(f);
    return *this;
  }

  virtual ~Bloom_Filter()
  {
    if (seeds)
      delete [] seeds;
  }

  auto & insert(const T & item) noexcept
  {
    const size_t & m = bits.size();
    for (auto seed = fst_seed; seed < last_seed; ++seed)
      bits.fast_write((*hash_fct)(item, *seed) % m, 1);

    ++num_ins;

    return *this;
  }

  auto & append(const T & item) noexcept { return insert(item); }

  bool contains(const T & item) const noexcept
  {
    const size_t & m = bits.size();
    for (auto seed = fst_seed; seed < last_seed; ++seed)
      if (not bits.fast_read((*hash_fct)(item, *seed) % m))
	return false;

    return true;
  }

  DynList<size_t> hash_seeds() const
  {
    DynList<size_t> ret;
    for (auto seed = fst_seed; seed < last_seed; ++seed)
      ret.append(*seed);
    return ret;
  }

  /// return the index that woulkd be hashed by item
  DynList<size_t> hashes(const T & item) const
  {
    DynList<size_t> ret;
    const size_t & m = bits.size();
    for (auto seed = fst_seed; seed < last_seed; ++seed)
      ret.append((*hash_fct)(item, *seed) % m);
    return ret;
  }

  DynList<size_t> common_hashes(const T & i1, const T & i2) const
  {
    return intercept(hashes(i1), hashes(i2));    
  }

  /// return the indexes of bits set to 1
  DynList<size_t> set_bits() const
  {
    DynList<size_t> ret;
    for (auto i = 0; i < bits.size(); ++i)
      if (bits(i) == 1)
	ret.append(i);
    
    return ret;
  }

  bool have_same_hashes(const Bloom_Filter & f) const noexcept
  {
    if (bits.size() != f.bits.size() or num_hash != f.num_hash)
      return false;
    for (auto seed1 = fst_seed, seed2 = f.fst_seed; seed1 < last_seed; 
	 ++seed1, ++seed2)
      if (*seed1 != *seed2)
	return false;
    return true;
  }

  bool operator == (const Bloom_Filter & f) const noexcept
  {
    return num_ins == f.num_ins and have_same_hashes(f) and eq(bits, f.bits);
  }

  bool operator != (const Bloom_Filter & f) const noexcept
  {
    return not (*this == f);
  }

  /// return number of bits set to 1
  size_t get_x() const noexcept
  {
    return bits.foldl<size_t>(0, [] (auto acu, auto bit)
			      {
				return bit ? acu + 1 : acu;
			      });
  }

  size_t expected_size(const size_t x) const noexcept 
  {
    auto m = size();
    auto k = get_k();
    return ceil(-(m*log(1- 1.0*x/m))/k); 
  }

  size_t expected_size() const noexcept 
  {
    return expected_size(get_x());
  }

  auto & operator |= (const Bloom_Filter & f) 
  {
    if (not have_same_hashes(f))
      throw std::domain_error("Bloom filter have differents hashes");

    bits |= f.bits;

    const auto & m = bits.size();
    const auto & k = num_hash;
    const auto x   = get_x();

    num_ins = expected_size(x);

    return *this;
  }

  auto & operator &= (const Bloom_Filter & f) 
  {
    if (not have_same_hashes(f))
      throw std::domain_error("Bloom filter have differents hashes");

    bits &= f.bits;

    const auto & m = bits.size();
    const auto & k = num_hash;
    num_ins = num_ins + f.num_ins - expected_size();

    return *this;
  }

      friend Bloom_Filter
  operator | (const Bloom_Filter & f1, const Bloom_Filter & f2) 
  {
    Bloom_Filter ret = f1;
    ret |= f2;
    return ret;
  }

      friend Bloom_Filter
  operator & (const Bloom_Filter & f1, const Bloom_Filter & f2) 
  {
    Bloom_Filter ret = f1;
    ret &= f2;
    return ret;
  }
};


} // end namespace Aleph

# endif // BLOOM_FILTER_H
