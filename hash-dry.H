# ifndef HASH_DRY_H
# define HASH_DRY_H

# include <ahFunction.H>


# define OHASH_COMMON(class_name)					\
  public:								\
  									\
  Hash_Fct get_hash_fct() const noexcept { return hash_fct; }		\
									\
  private:								\
  									\
  Key * test_resize(Bucket * curr_bucket, const Key & key)		\
  {									\
    if (not ((with_resize and this->current_alpha() >= upper_alpha)))	\
      return &curr_bucket->key;						\
									\
    Key key_copy = key;							\
    this->resize(Primes::next_prime(2*len));				\
									\
    Key * key_ptr = search(key_copy);					\
    assert(key_ptr != nullptr);						\
    assert(*key_ptr == key_copy);					\
									\
    return key_ptr;							\
  }									\
									\
 static void update_stat_len(DynArray<size_t> & lens, size_t i)		\
 {									\
   if (lens.exist(i))							\
     lens(i) += 1;							\
   else									\
     lens.touch(i) = 1;							\
 }									\
									\
public:									\
									\
 void set_hash_fct(Hash_Fct fct) noexcept				\
 {									\
   hash_fct = fct;							\
 }

namespace Aleph
{

  extern const float hash_default_lower_alpha;

  extern const float hash_default_upper_alpha;

}


# endif // HASH_DRY_H
