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

/** @file tpl_paged_value_codec.H
 *  @brief Fixed-size portable codecs for paged persistent tree payloads.
 *
 *  This header centralizes the little-endian fixed-size serialization used by
 *  Aleph's paged persistent B-Tree and B+ Tree implementations. The default
 *  codecs cover integral types, enums, IEEE-754 `float`/`double`, and
 *  `std::array` compositions thereof.
 *
 *  @ingroup Trees
 */

# ifndef TPL_PAGED_VALUE_CODEC_H
# define TPL_PAGED_VALUE_CODEC_H

# include <array>
# include <bit>
# include <concepts>
# include <cstddef>
# include <cstdint>
# include <cstring>
# include <limits>
# include <string>
# include <type_traits>

# include <tpl_paged_tree_durability.H>

namespace Aleph
{
  namespace detail
  {
    template <typename T>
    struct Paged_Value_Codec;

    template <typename Codec, typename Value>
    concept Fixed_Paged_Value_Codec = requires(const Value & value,
                                               unsigned char * out,
                                               const unsigned char * in,
                                               std::uint32_t crc)
    {
      { Codec::storage_id } -> std::convertible_to<std::uint32_t>;
      { Codec::encoded_size } -> std::convertible_to<size_t>;
      { Codec::encode(value, out) } -> std::same_as<void>;
      { Codec::decode(in) } -> std::same_as<Value>;
      { Codec::add_to_crc(crc, value) } -> std::same_as<std::uint32_t>;
    };

    [[nodiscard]] constexpr std::uint32_t mix_codec_id(
      const std::uint32_t seed,
      const std::uint32_t value) noexcept
    {
      return (seed ^ value) * 16777619u + 0x9E3779B9u;
    }

    template <typename UInt>
      requires(std::is_unsigned_v<UInt>)
    inline void encode_unsigned_le(UInt value, unsigned char * out) noexcept
    {
      for (size_t i = 0; i < sizeof(UInt); ++i)
        out[i] = static_cast<unsigned char>((value >> (i * 8)) & 0xFFu);
    }

    template <typename UInt>
      requires(std::is_unsigned_v<UInt>)
    [[nodiscard]] inline UInt decode_unsigned_le(
      const unsigned char * in) noexcept
    {
      UInt value = 0;
      for (size_t i = 0; i < sizeof(UInt); ++i)
        value |= static_cast<UInt>(in[i]) << (i * 8);
      return value;
    }

    template <typename T>
      requires(std::is_integral_v<T> and sizeof(T) == 1)
    struct Paged_Value_Codec<T>
    {
      static constexpr std::uint32_t storage_id =
        0x11000000u | (std::is_signed_v<T> ? 1u : 0u);
      static constexpr size_t encoded_size = 1;

      static void encode(const T value, unsigned char * out) noexcept
      {
        out[0] = static_cast<unsigned char>(value);
      }

      [[nodiscard]] static T decode(const unsigned char * in) noexcept
      {
        if constexpr (std::is_same_v<T, bool>)
          return in[0] != 0;
        else
          return static_cast<T>(in[0]);
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const T value) noexcept
      {
        unsigned char byte = 0;
        encode(value, &byte);
        return crc32_add_bytes(crc, &byte, 1);
      }
    };

    template <typename T>
      requires(std::is_integral_v<T> and sizeof(T) != 1)
    struct Paged_Value_Codec<T>
    {
      using unsigned_type = std::make_unsigned_t<T>;

      static constexpr std::uint32_t storage_id =
        0x12000000u | (static_cast<std::uint32_t>(sizeof(T)) << 8)
        | (std::is_signed_v<T> ? 1u : 0u);
      static constexpr size_t encoded_size = sizeof(T);

      static void encode(const T value, unsigned char * out) noexcept
      {
        unsigned_type bits = 0;
        std::memcpy(&bits, &value, sizeof(T));
        encode_unsigned_le(bits, out);
      }

      [[nodiscard]] static T decode(const unsigned char * in) noexcept
      {
        const unsigned_type bits = decode_unsigned_le<unsigned_type>(in);
        T value{};
        std::memcpy(&value, &bits, sizeof(T));
        return value;
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const T value) noexcept
      {
        std::array<unsigned char, encoded_size> bytes = {};
        encode(value, bytes.data());
        return crc32_add_bytes(crc, bytes.data(), bytes.size());
      }
    };

    template <typename T>
      requires std::is_enum_v<T>
    struct Paged_Value_Codec<T>
    {
      using underlying_type = std::underlying_type_t<T>;
      using underlying_codec = Paged_Value_Codec<underlying_type>;

      static constexpr std::uint32_t storage_id =
        mix_codec_id(0x21000000u, underlying_codec::storage_id);
      static constexpr size_t encoded_size = underlying_codec::encoded_size;

      static void encode(const T value, unsigned char * out) noexcept
      {
        underlying_codec::encode(static_cast<underlying_type>(value), out);
      }

      [[nodiscard]] static T decode(const unsigned char * in) noexcept
      {
        return static_cast<T>(underlying_codec::decode(in));
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const T value) noexcept
      {
        return underlying_codec::add_to_crc(
          crc, static_cast<underlying_type>(value));
      }
    };

    template <typename T>
      requires(std::is_floating_point_v<T> and std::numeric_limits<T>::is_iec559
               and (sizeof(T) == 4 or sizeof(T) == 8))
    struct Paged_Value_Codec<T>
    {
      using unsigned_type =
        std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;

      static constexpr std::uint32_t storage_id =
        0x31000000u | static_cast<std::uint32_t>(sizeof(T));
      static constexpr size_t encoded_size = sizeof(T);

      static void encode(const T value, unsigned char * out) noexcept
      {
        const auto bits = std::bit_cast<unsigned_type>(value);
        encode_unsigned_le(bits, out);
      }

      [[nodiscard]] static T decode(const unsigned char * in) noexcept
      {
        const auto bits = decode_unsigned_le<unsigned_type>(in);
        return std::bit_cast<T>(bits);
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const T value) noexcept
      {
        std::array<unsigned char, encoded_size> bytes = {};
        encode(value, bytes.data());
        return crc32_add_bytes(crc, bytes.data(), bytes.size());
      }
    };

    template <typename T, size_t N>
    struct Paged_Value_Codec<std::array<T, N>>
    {
      using value_type = T;
      using element_codec = Paged_Value_Codec<T>;

      static constexpr std::uint32_t storage_id =
        mix_codec_id(mix_codec_id(0x41000000u, element_codec::storage_id),
                     static_cast<std::uint32_t>(N));
      static constexpr size_t encoded_size = N * element_codec::encoded_size;

      static void encode(const std::array<T, N> & value,
                         unsigned char * out) noexcept
      {
        for (size_t i = 0; i < N; ++i)
          element_codec::encode(value[i],
                                out + i * element_codec::encoded_size);
      }

      [[nodiscard]] static std::array<T, N>
      decode(const unsigned char * in) noexcept
      {
        std::array<T, N> value = {};
        for (size_t i = 0; i < N; ++i)
          value[i] = element_codec::decode(in + i * element_codec::encoded_size);
        return value;
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        std::uint32_t crc,
        const std::array<T, N> & value) noexcept
      {
        for (size_t i = 0; i < N; ++i)
          crc = element_codec::add_to_crc(crc, value[i]);
        return crc;
      }
    };

    template <size_t Capacity, typename SizeType = std::uint32_t>
      requires(std::is_unsigned_v<SizeType>)
    struct Paged_Bounded_String_Codec
    {
      static_assert(Capacity > 0,
                    "Paged_Bounded_String_Codec requires Capacity > 0");

      static constexpr std::uint32_t storage_id =
        mix_codec_id(mix_codec_id(0x61000000u,
                                  static_cast<std::uint32_t>(Capacity)),
                     static_cast<std::uint32_t>(sizeof(SizeType)));
      static constexpr size_t encoded_size = sizeof(SizeType) + Capacity;

      static void encode(const std::string & value, unsigned char * out)
      {
        ah_runtime_error_unless(value.size() <= Capacity)
          << "Paged_Bounded_String_Codec<" << Capacity
          << ">::encode(): string size " << value.size()
          << " exceeds capacity " << Capacity;

        encode_unsigned_le(static_cast<SizeType>(value.size()), out);
        std::memset(out + sizeof(SizeType), 0, Capacity);
        if (not value.empty())
          std::memcpy(out + sizeof(SizeType), value.data(), value.size());
      }

      [[nodiscard]] static std::string decode(const unsigned char * in)
      {
        const auto size = decode_unsigned_le<SizeType>(in);
        ah_runtime_error_unless(size <= Capacity)
          << "Paged_Bounded_String_Codec<" << Capacity
          << ">::decode(): corrupted size " << size
          << " exceeds capacity " << Capacity;

        return std::string(reinterpret_cast<const char *>(in + sizeof(SizeType)),
                           static_cast<size_t>(size));
      }

      [[nodiscard]] static std::uint32_t add_to_crc(
        const std::uint32_t crc,
        const std::string & value)
      {
        std::array<unsigned char, encoded_size> bytes = {};
        encode(value, bytes.data());
        return crc32_add_bytes(crc, bytes.data(), bytes.size());
      }
    };
  } // namespace detail
} // namespace Aleph

# endif // TPL_PAGED_VALUE_CODEC_H
