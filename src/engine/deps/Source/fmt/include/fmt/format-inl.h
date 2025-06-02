// Formatting library for C++ - implementation
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_FORMAT_INL_H_
#define FMT_FORMAT_INL_H_

#include <algorithm>
#include <cctype>
#include <cerrno>  // errno
#include <climits>
#include <cmath>
#include <cstdarg>
#include <cstring>  // std::memmove
#include <cwchar>
#include <exception>

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
#  include <locale>
#endif

#ifdef _WIN32
#  include <io.h>  // _isatty
#endif

#include "format.h"

FMT_BEGIN_NAMESPACE
namespace detail {

FMT_FUNC void assert_fail(const char* file, int line, const char* message) {
  // Use unchecked std::fprintf to avoid triggering another assertion when
  // writing to stderr fails
  std::fprintf(stderr, "%s:%d: assertion failed: %s", file, line, message);
  // Chosen instead of std::abort to satisfy Clang in CUDA mode during device
  // code pass.
  std::terminate();
}

FMT_FUNC void throw_format_error(const char* message) {
  FMT_THROW(format_error(message));
}

#ifndef _MSC_VER
#  define FMT_SNPRINTF snprintf
#else  // _MSC_VER
inline int fmt_snprintf(char* buffer, size_t size, const char* format, ...) {
  va_list args;
  va_start(args, format);
  int result = vsnprintf_s(buffer, size, _TRUNCATE, format, args);
  va_end(args);
  return result;
}
#  define FMT_SNPRINTF fmt_snprintf
#endif  // _MSC_VER

FMT_FUNC void format_error_code(detail::buffer<char>& out, int error_code,
                                string_view message) FMT_NOEXCEPT {
  // Report error code making sure that the output fits into
  // inline_buffer_size to avoid dynamic memory allocation and potential
  // bad_alloc.
  out.try_resize(0);
  static const char SEP[] = ": ";
  static const char ERROR_STR[] = "error ";
  // Subtract 2 to account for terminating null characters in SEP and ERROR_STR.
  size_t error_code_size = sizeof(SEP) + sizeof(ERROR_STR) - 2;
  auto abs_value = static_cast<uint32_or_64_or_128_t<int>>(error_code);
  if (detail::is_negative(error_code)) {
    abs_value = 0 - abs_value;
    ++error_code_size;
  }
  error_code_size += detail::to_unsigned(detail::count_digits(abs_value));
  auto it = buffer_appender<char>(out);
  if (message.size() <= inline_buffer_size - error_code_size)
    format_to(it, FMT_STRING("{}{}"), message, SEP);
  format_to(it, FMT_STRING("{}{}"), ERROR_STR, error_code);
  FMT_ASSERT(out.size() <= inline_buffer_size, "");
}

FMT_FUNC void report_error(format_func func, int error_code,
                           const char* message) FMT_NOEXCEPT {
  memory_buffer full_message;
  func(full_message, error_code, message);
  // Don't use fwrite_fully because the latter may throw.
  if (std::fwrite(full_message.data(), full_message.size(), 1, stderr) > 0)
    std::fputc('\n', stderr);
}

// A wrapper around fwrite that throws on error.
inline void fwrite_fully(const void* ptr, size_t size, size_t count,
                         FILE* stream) {
  size_t written = std::fwrite(ptr, size, count, stream);
  if (written < count) FMT_THROW(system_error(errno, "cannot write to file"));
}

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template <typename Locale>
locale_ref::locale_ref(const Locale& loc) : locale_(&loc) {
  static_assert(std::is_same<Locale, std::locale>::value, "");
}

template <typename Locale> Locale locale_ref::get() const {
  static_assert(std::is_same<Locale, std::locale>::value, "");
  return locale_ ? *static_cast<const std::locale*>(locale_) : std::locale();
}

template <typename Char>
FMT_FUNC auto thousands_sep_impl(locale_ref loc) -> thousands_sep_result<Char> {
  auto& facet = std::use_facet<std::numpunct<Char>>(loc.get<std::locale>());
  auto grouping = facet.grouping();
  auto thousands_sep = grouping.empty() ? Char() : facet.thousands_sep();
  return {std::move(grouping), thousands_sep};
}
template <typename Char> FMT_FUNC Char decimal_point_impl(locale_ref loc) {
  return std::use_facet<std::numpunct<Char>>(loc.get<std::locale>())
      .decimal_point();
}
#else
template <typename Char>
FMT_FUNC auto thousands_sep_impl(locale_ref) -> thousands_sep_result<Char> {
  return {"\03", FMT_STATIC_THOUSANDS_SEPARATOR};
}
template <typename Char> FMT_FUNC Char decimal_point_impl(locale_ref) {
  return '.';
}
#endif
}  // namespace detail

#if !FMT_MSC_VER
FMT_API FMT_FUNC format_error::~format_error() FMT_NOEXCEPT = default;
#endif

FMT_FUNC std::system_error vsystem_error(int error_code, string_view format_str,
                                         format_args args) {
  auto ec = std::error_code(error_code, std::generic_category());
  return std::system_error(ec, vformat(format_str, args));
}

namespace detail {

template <> FMT_FUNC int count_digits<4>(detail::fallback_uintptr n) {
  // fallback_uintptr is always stored in little endian.
  int i = static_cast<int>(sizeof(void*)) - 1;
  while (i > 0 && n.value[i] == 0) --i;
  auto char_digits = std::numeric_limits<unsigned char>::digits / 4;
  return i >= 0 ? i * char_digits + count_digits<4, unsigned>(n.value[i]) : 1;
}

// log10(2) = 0x0.4d104d427de7fbcc...
static constexpr uint64_t log10_2_significand = 0x4d104d427de7fbcc;

template <typename T = void> struct basic_impl_data {
  // Normalized 64-bit significands of pow(10, k), for k = -348, -340, ..., 340.
  // These are generated by support/compute-powers.py.
  static constexpr uint64_t pow10_significands[87] = {
      0xfa8fd5a0081c0288, 0xbaaee17fa23ebf76, 0x8b16fb203055ac76,
      0xcf42894a5dce35ea, 0x9a6bb0aa55653b2d, 0xe61acf033d1a45df,
      0xab70fe17c79ac6ca, 0xff77b1fcbebcdc4f, 0xbe5691ef416bd60c,
      0x8dd01fad907ffc3c, 0xd3515c2831559a83, 0x9d71ac8fada6c9b5,
      0xea9c227723ee8bcb, 0xaecc49914078536d, 0x823c12795db6ce57,
      0xc21094364dfb5637, 0x9096ea6f3848984f, 0xd77485cb25823ac7,
      0xa086cfcd97bf97f4, 0xef340a98172aace5, 0xb23867fb2a35b28e,
      0x84c8d4dfd2c63f3b, 0xc5dd44271ad3cdba, 0x936b9fcebb25c996,
      0xdbac6c247d62a584, 0xa3ab66580d5fdaf6, 0xf3e2f893dec3f126,
      0xb5b5ada8aaff80b8, 0x87625f056c7c4a8b, 0xc9bcff6034c13053,
      0x964e858c91ba2655, 0xdff9772470297ebd, 0xa6dfbd9fb8e5b88f,
      0xf8a95fcf88747d94, 0xb94470938fa89bcf, 0x8a08f0f8bf0f156b,
      0xcdb02555653131b6, 0x993fe2c6d07b7fac, 0xe45c10c42a2b3b06,
      0xaa242499697392d3, 0xfd87b5f28300ca0e, 0xbce5086492111aeb,
      0x8cbccc096f5088cc, 0xd1b71758e219652c, 0x9c40000000000000,
      0xe8d4a51000000000, 0xad78ebc5ac620000, 0x813f3978f8940984,
      0xc097ce7bc90715b3, 0x8f7e32ce7bea5c70, 0xd5d238a4abe98068,
      0x9f4f2726179a2245, 0xed63a231d4c4fb27, 0xb0de65388cc8ada8,
      0x83c7088e1aab65db, 0xc45d1df942711d9a, 0x924d692ca61be758,
      0xda01ee641a708dea, 0xa26da3999aef774a, 0xf209787bb47d6b85,
      0xb454e4a179dd1877, 0x865b86925b9bc5c2, 0xc83553c5c8965d3d,
      0x952ab45cfa97a0b3, 0xde469fbd99a05fe3, 0xa59bc234db398c25,
      0xf6c69a72a3989f5c, 0xb7dcbf5354e9bece, 0x88fcf317f22241e2,
      0xcc20ce9bd35c78a5, 0x98165af37b2153df, 0xe2a0b5dc971f303a,
      0xa8d9d1535ce3b396, 0xfb9b7cd9a4a7443c, 0xbb764c4ca7a44410,
      0x8bab8eefb6409c1a, 0xd01fef10a657842c, 0x9b10a4e5e9913129,
      0xe7109bfba19c0c9d, 0xac2820d9623bf429, 0x80444b5e7aa7cf85,
      0xbf21e44003acdd2d, 0x8e679c2f5e44ff8f, 0xd433179d9c8cb841,
      0x9e19db92b4e31ba9, 0xeb96bf6ebadf77d9, 0xaf87023b9bf0ee6b,
  };

#if FMT_GCC_VERSION && FMT_GCC_VERSION < 409
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wnarrowing"
#endif
  // Binary exponents of pow(10, k), for k = -348, -340, ..., 340, corresponding
  // to significands above.
  static constexpr int16_t pow10_exponents[87] = {
      -1220, -1193, -1166, -1140, -1113, -1087, -1060, -1034, -1007, -980, -954,
      -927,  -901,  -874,  -847,  -821,  -794,  -768,  -741,  -715,  -688, -661,
      -635,  -608,  -582,  -555,  -529,  -502,  -475,  -449,  -422,  -396, -369,
      -343,  -316,  -289,  -263,  -236,  -210,  -183,  -157,  -130,  -103, -77,
      -50,   -24,   3,     30,    56,    83,    109,   136,   162,   189,  216,
      242,   269,   295,   322,   348,   375,   402,   428,   455,   481,  508,
      534,   561,   588,   614,   641,   667,   694,   720,   747,   774,  800,
      827,   853,   880,   907,   933,   960,   986,   1013,  1039,  1066};
#if FMT_GCC_VERSION && FMT_GCC_VERSION < 409
#  pragma GCC diagnostic pop
#endif

  static constexpr uint64_t power_of_10_64[20] = {
      1, FMT_POWERS_OF_10(1ULL), FMT_POWERS_OF_10(1000000000ULL),
      10000000000000000000ULL};
};

// This is a struct rather than an alias to avoid shadowing warnings in gcc.
struct impl_data : basic_impl_data<> {};

#if __cplusplus < 201703L
template <typename T>
constexpr uint64_t basic_impl_data<T>::pow10_significands[];
template <typename T> constexpr int16_t basic_impl_data<T>::pow10_exponents[];
template <typename T> constexpr uint64_t basic_impl_data<T>::power_of_10_64[];
#endif

template <typename T> struct bits {
  static FMT_CONSTEXPR_DECL const int value =
      static_cast<int>(sizeof(T) * std::numeric_limits<unsigned char>::digits);
};

// Returns the number of significand bits in Float excluding the implicit bit.
template <typename Float> constexpr int num_significand_bits() {
  // Subtract 1 to account for an implicit most significant bit in the
  // normalized form.
  return std::numeric_limits<Float>::digits - 1;
}

// A floating-point number f * pow(2, e).
struct fp {
  uint64_t f;
  int e;

  static constexpr const int num_significand_bits = bits<decltype(f)>::value;

  constexpr fp() : f(0), e(0) {}
  constexpr fp(uint64_t f_val, int e_val) : f(f_val), e(e_val) {}

  // Constructs fp from an IEEE754 floating-point number. It is a template to
  // prevent compile errors on systems where n is not IEEE754.
  template <typename Float> explicit FMT_CONSTEXPR fp(Float n) { assign(n); }

  template <typename Float>
  using is_supported = bool_constant<sizeof(Float) == sizeof(uint64_t) ||
                                     sizeof(Float) == sizeof(uint32_t)>;

  // Assigns d to this and return true iff predecessor is closer than successor.
  template <typename Float, FMT_ENABLE_IF(is_supported<Float>::value)>
  FMT_CONSTEXPR bool assign(Float n) {
    // Assume float is in the format [sign][exponent][significand].
    const int num_float_significand_bits =
        detail::num_significand_bits<Float>();
    const uint64_t implicit_bit = 1ULL << num_float_significand_bits;
    const uint64_t significand_mask = implicit_bit - 1;
    constexpr bool is_double = sizeof(Float) == sizeof(uint64_t);
    auto u = bit_cast<conditional_t<is_double, uint64_t, uint32_t>>(n);
    f = u & significand_mask;
    const uint64_t exponent_mask = (~0ULL >> 1) & ~significand_mask;
    int biased_e =
        static_cast<int>((u & exponent_mask) >> num_float_significand_bits);
    // The predecessor is closer if n is a normalized power of 2 (f == 0) other
    // than the smallest normalized number (biased_e > 1).
    bool is_predecessor_closer = f == 0 && biased_e > 1;
    if (biased_e != 0)
      f += implicit_bit;
    else
      biased_e = 1;  // Subnormals use biased exponent 1 (min exponent).
    const int exponent_bias = std::numeric_limits<Float>::max_exponent - 1;
    e = biased_e - exponent_bias - num_float_significand_bits;
    return is_predecessor_closer;
  }

  template <typename Float, FMT_ENABLE_IF(!is_supported<Float>::value)>
  bool assign(Float) {
    FMT_ASSERT(false, "");
    return false;
  }
};

// Normalizes the value converted from double and multiplied by (1 << SHIFT).
template <int SHIFT = 0> FMT_CONSTEXPR fp normalize(fp value) {
  // Handle subnormals.
  const uint64_t implicit_bit = 1ULL << num_significand_bits<double>();
  const auto shifted_implicit_bit = implicit_bit << SHIFT;
  while ((value.f & shifted_implicit_bit) == 0) {
    value.f <<= 1;
    --value.e;
  }
  // Subtract 1 to account for hidden bit.
  const auto offset =
      fp::num_significand_bits - num_significand_bits<double>() - SHIFT - 1;
  value.f <<= offset;
  value.e -= offset;
  return value;
}

inline bool operator==(fp x, fp y) { return x.f == y.f && x.e == y.e; }

// Computes lhs * rhs / pow(2, 64) rounded to nearest with half-up tie breaking.
FMT_CONSTEXPR inline uint64_t multiply(uint64_t lhs, uint64_t rhs) {
#if FMT_USE_INT128
  auto product = static_cast<__uint128_t>(lhs) * rhs;
  auto f = static_cast<uint64_t>(product >> 64);
  return (static_cast<uint64_t>(product) & (1ULL << 63)) != 0 ? f + 1 : f;
#else
  // Multiply 32-bit parts of significands.
  uint64_t mask = (1ULL << 32) - 1;
  uint64_t a = lhs >> 32, b = lhs & mask;
  uint64_t c = rhs >> 32, d = rhs & mask;
  uint64_t ac = a * c, bc = b * c, ad = a * d, bd = b * d;
  // Compute mid 64-bit of result and round.
  uint64_t mid = (bd >> 32) + (ad & mask) + (bc & mask) + (1U << 31);
  return ac + (ad >> 32) + (bc >> 32) + (mid >> 32);
#endif
}

FMT_CONSTEXPR inline fp operator*(fp x, fp y) {
  return {multiply(x.f, y.f), x.e + y.e + 64};
}

// Returns a cached power of 10 `c_k = c_k.f * pow(2, c_k.e)` such that its
// (binary) exponent satisfies `min_exponent <= c_k.e <= min_exponent + 28`.
FMT_CONSTEXPR inline fp get_cached_power(int min_exponent,
                                         int& pow10_exponent) {
  const int shift = 32;
  const auto significand = static_cast<int64_t>(log10_2_significand);
  int index = static_cast<int>(
      ((min_exponent + fp::num_significand_bits - 1) * (significand >> shift) +
       ((int64_t(1) << shift) - 1))  // ceil
      >> 32                          // arithmetic shift
  );
  // Decimal exponent of the first (smallest) cached power of 10.
  const int first_dec_exp = -348;
  // Difference between 2 consecutive decimal exponents in cached powers of 10.
  const int dec_exp_step = 8;
  index = (index - first_dec_exp - 1) / dec_exp_step + 1;
  pow10_exponent = first_dec_exp + index * dec_exp_step;
  return {impl_data::pow10_significands[index],
          impl_data::pow10_exponents[index]};
}

// A simple accumulator to hold the sums of terms in bigint::square if uint128_t
// is not available.
struct accumulator {
  uint64_t lower;
  uint64_t upper;

  constexpr accumulator() : lower(0), upper(0) {}
  constexpr explicit operator uint32_t() const {
    return static_cast<uint32_t>(lower);
  }

  FMT_CONSTEXPR void operator+=(uint64_t n) {
    lower += n;
    if (lower < n) ++upper;
  }
  FMT_CONSTEXPR void operator>>=(int shift) {
    FMT_ASSERT(shift == 32, "");
    (void)shift;
    lower = (upper << 32) | (lower >> 32);
    upper >>= 32;
  }
};

class bigint {
 private:
  // A bigint is stored as an array of bigits (big digits), with bigit at index
  // 0 being the least significant one.
  using bigit = uint32_t;
  using double_bigit = uint64_t;
  enum { bigits_capacity = 32 };
  basic_memory_buffer<bigit, bigits_capacity> bigits_;
  int exp_;

  FMT_CONSTEXPR20 bigit operator[](int index) const {
    return bigits_[to_unsigned(index)];
  }
  FMT_CONSTEXPR20 bigit& operator[](int index) {
    return bigits_[to_unsigned(index)];
  }

  static FMT_CONSTEXPR_DECL const int bigit_bits = bits<bigit>::value;

  friend struct formatter<bigint>;

  FMT_CONSTEXPR20 void subtract_bigits(int index, bigit other, bigit& borrow) {
    auto result = static_cast<double_bigit>((*this)[index]) - other - borrow;
    (*this)[index] = static_cast<bigit>(result);
    borrow = static_cast<bigit>(result >> (bigit_bits * 2 - 1));
  }

  FMT_CONSTEXPR20 void remove_leading_zeros() {
    int num_bigits = static_cast<int>(bigits_.size()) - 1;
    while (num_bigits > 0 && (*this)[num_bigits] == 0) --num_bigits;
    bigits_.resize(to_unsigned(num_bigits + 1));
  }

  // Computes *this -= other assuming aligned bigints and *this >= other.
  FMT_CONSTEXPR20 void subtract_aligned(const bigint& other) {
    FMT_ASSERT(other.exp_ >= exp_, "unaligned bigints");
    FMT_ASSERT(compare(*this, other) >= 0, "");
    bigit borrow = 0;
    int i = other.exp_ - exp_;
    for (size_t j = 0, n = other.bigits_.size(); j != n; ++i, ++j)
      subtract_bigits(i, other.bigits_[j], borrow);
    while (borrow > 0) subtract_bigits(i, 0, borrow);
    remove_leading_zeros();
  }

  FMT_CONSTEXPR20 void multiply(uint32_t value) {
    const double_bigit wide_value = value;
    bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      double_bigit result = bigits_[i] * wide_value + carry;
      bigits_[i] = static_cast<bigit>(result);
      carry = static_cast<bigit>(result >> bigit_bits);
    }
    if (carry != 0) bigits_.push_back(carry);
  }

  FMT_CONSTEXPR20 void multiply(uint64_t value) {
    const bigit mask = ~bigit(0);
    const double_bigit lower = value & mask;
    const double_bigit upper = value >> bigit_bits;
    double_bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      double_bigit result = bigits_[i] * lower + (carry & mask);
      carry =
          bigits_[i] * upper + (result >> bigit_bits) + (carry >> bigit_bits);
      bigits_[i] = static_cast<bigit>(result);
    }
    while (carry != 0) {
      bigits_.push_back(carry & mask);
      carry >>= bigit_bits;
    }
  }

 public:
  FMT_CONSTEXPR20 bigint() : exp_(0) {}
  explicit bigint(uint64_t n) { assign(n); }
  FMT_CONSTEXPR20 ~bigint() {
    FMT_ASSERT(bigits_.capacity() <= bigits_capacity, "");
  }

  bigint(const bigint&) = delete;
  void operator=(const bigint&) = delete;

  FMT_CONSTEXPR20 void assign(const bigint& other) {
    auto size = other.bigits_.size();
    bigits_.resize(size);
    auto data = other.bigits_.data();
    std::copy(data, data + size, make_checked(bigits_.data(), size));
    exp_ = other.exp_;
  }

  FMT_CONSTEXPR20 void assign(uint64_t n) {
    size_t num_bigits = 0;
    do {
      bigits_[num_bigits++] = n & ~bigit(0);
      n >>= bigit_bits;
    } while (n != 0);
    bigits_.resize(num_bigits);
    exp_ = 0;
  }

  FMT_CONSTEXPR20 int num_bigits() const {
    return static_cast<int>(bigits_.size()) + exp_;
  }

  FMT_NOINLINE FMT_CONSTEXPR20 bigint& operator<<=(int shift) {
    FMT_ASSERT(shift >= 0, "");
    exp_ += shift / bigit_bits;
    shift %= bigit_bits;
    if (shift == 0) return *this;
    bigit carry = 0;
    for (size_t i = 0, n = bigits_.size(); i < n; ++i) {
      bigit c = bigits_[i] >> (bigit_bits - shift);
      bigits_[i] = (bigits_[i] << shift) + carry;
      carry = c;
    }
    if (carry != 0) bigits_.push_back(carry);
    return *this;
  }

  template <typename Int> FMT_CONSTEXPR20 bigint& operator*=(Int value) {
    FMT_ASSERT(value > 0, "");
    multiply(uint32_or_64_or_128_t<Int>(value));
    return *this;
  }

  friend FMT_CONSTEXPR20 int compare(const bigint& lhs, const bigint& rhs) {
    int num_lhs_bigits = lhs.num_bigits(), num_rhs_bigits = rhs.num_bigits();
    if (num_lhs_bigits != num_rhs_bigits)
      return num_lhs_bigits > num_rhs_bigits ? 1 : -1;
    int i = static_cast<int>(lhs.bigits_.size()) - 1;
    int j = static_cast<int>(rhs.bigits_.size()) - 1;
    int end = i - j;
    if (end < 0) end = 0;
    for (; i >= end; --i, --j) {
      bigit lhs_bigit = lhs[i], rhs_bigit = rhs[j];
      if (lhs_bigit != rhs_bigit) return lhs_bigit > rhs_bigit ? 1 : -1;
    }
    if (i != j) return i > j ? 1 : -1;
    return 0;
  }

  // Returns compare(lhs1 + lhs2, rhs).
  friend FMT_CONSTEXPR20 int add_compare(const bigint& lhs1, const bigint& lhs2,
                                         const bigint& rhs) {
    int max_lhs_bigits = (std::max)(lhs1.num_bigits(), lhs2.num_bigits());
    int num_rhs_bigits = rhs.num_bigits();
    if (max_lhs_bigits + 1 < num_rhs_bigits) return -1;
    if (max_lhs_bigits > num_rhs_bigits) return 1;
    auto get_bigit = [](const bigint& n, int i) -> bigit {
      return i >= n.exp_ && i < n.num_bigits() ? n[i - n.exp_] : 0;
    };
    double_bigit borrow = 0;
    int min_exp = (std::min)((std::min)(lhs1.exp_, lhs2.exp_), rhs.exp_);
    for (int i = num_rhs_bigits - 1; i >= min_exp; --i) {
      double_bigit sum =
          static_cast<double_bigit>(get_bigit(lhs1, i)) + get_bigit(lhs2, i);
      bigit rhs_bigit = get_bigit(rhs, i);
      if (sum > rhs_bigit + borrow) return 1;
      borrow = rhs_bigit + borrow - sum;
      if (borrow > 1) return -1;
      borrow <<= bigit_bits;
    }
    return borrow != 0 ? -1 : 0;
  }

  // Assigns pow(10, exp) to this bigint.
  FMT_CONSTEXPR20 void assign_pow10(int exp) {
    FMT_ASSERT(exp >= 0, "");
    if (exp == 0) return assign(1);
    // Find the top bit.
    int bitmask = 1;
    while (exp >= bitmask) bitmask <<= 1;
    bitmask >>= 1;
    // pow(10, exp) = pow(5, exp) * pow(2, exp). First compute pow(5, exp) by
    // repeated squaring and multiplication.
    assign(5);
    bitmask >>= 1;
    while (bitmask != 0) {
      square();
      if ((exp & bitmask) != 0) *this *= 5;
      bitmask >>= 1;
    }
    *this <<= exp;  // Multiply by pow(2, exp) by shifting.
  }

  FMT_CONSTEXPR20 void square() {
    int num_bigits = static_cast<int>(bigits_.size());
    int num_result_bigits = 2 * num_bigits;
    basic_memory_buffer<bigit, bigits_capacity> n(std::move(bigits_));
    bigits_.resize(to_unsigned(num_result_bigits));
    using accumulator_t = conditional_t<FMT_USE_INT128, uint128_t, accumulator>;
    auto sum = accumulator_t();
    for (int bigit_index = 0; bigit_index < num_bigits; ++bigit_index) {
      // Compute bigit at position bigit_index of the result by adding
      // cross-product terms n[i] * n[j] such that i + j == bigit_index.
      for (int i = 0, j = bigit_index; j >= 0; ++i, --j) {
        // Most terms are multiplied twice which can be optimized in the future.
        sum += static_cast<double_bigit>(n[i]) * n[j];
      }
      (*this)[bigit_index] = static_cast<bigit>(sum);
      sum >>= bits<bigit>::value;  // Compute the carry.
    }
    // Do the same for the top half.
    for (int bigit_index = num_bigits; bigit_index < num_result_bigits;
         ++bigit_index) {
      for (int j = num_bigits - 1, i = bigit_index - j; i < num_bigits;)
        sum += static_cast<double_bigit>(n[i++]) * n[j--];
      (*this)[bigit_index] = static_cast<bigit>(sum);
      sum >>= bits<bigit>::value;
    }
    remove_leading_zeros();
    exp_ *= 2;
  }

  // If this bigint has a bigger exponent than other, adds trailing zero to make
  // exponents equal. This simplifies some operations such as subtraction.
  FMT_CONSTEXPR20 void align(const bigint& other) {
    int exp_difference = exp_ - other.exp_;
    if (exp_difference <= 0) return;
    int num_bigits = static_cast<int>(bigits_.size());
    bigits_.resize(to_unsigned(num_bigits + exp_difference));
    for (int i = num_bigits - 1, j = i + exp_difference; i >= 0; --i, --j)
      bigits_[j] = bigits_[i];
    std::uninitialized_fill_n(bigits_.data(), exp_difference, 0);
    exp_ -= exp_difference;
  }

  // Divides this bignum by divisor, assigning the remainder to this and
  // returning the quotient.
  FMT_CONSTEXPR20 int divmod_assign(const bigint& divisor) {
    FMT_ASSERT(this != &divisor, "");
    if (compare(*this, divisor) < 0) return 0;
    FMT_ASSERT(divisor.bigits_[divisor.bigits_.size() - 1u] != 0, "");
    align(divisor);
    int quotient = 0;
    do {
      subtract_aligned(divisor);
      ++quotient;
    } while (compare(*this, divisor) >= 0);
    return quotient;
  }
};

enum class round_direction { unknown, up, down };

// Given the divisor (normally a power of 10), the remainder = v % divisor for
// some number v and the error, returns whether v should be rounded up, down, or
// whether the rounding direction can't be determined due to error.
// error should be less than divisor / 2.
FMT_CONSTEXPR inline round_direction get_round_direction(uint64_t divisor,
                                                         uint64_t remainder,
                                                         uint64_t error) {
  FMT_ASSERT(remainder < divisor, "");  // divisor - remainder won't overflow.
  FMT_ASSERT(error < divisor, "");      // divisor - error won't overflow.
  FMT_ASSERT(error < divisor - error, "");  // error * 2 won't overflow.
  // Round down if (remainder + error) * 2 <= divisor.
  if (remainder <= divisor - remainder && error * 2 <= divisor - remainder * 2)
    return round_direction::down;
  // Round up if (remainder - error) * 2 >= divisor.
  if (remainder >= error &&
      remainder - error >= divisor - (remainder - error)) {
    return round_direction::up;
  }
  return round_direction::unknown;
}

namespace digits {
enum result {
  more,  // Generate more digits.
  done,  // Done generating digits.
  error  // Digit generation cancelled due to an error.
};
}

struct gen_digits_handler {
  char* buf;
  int size;
  int precision;
  int exp10;
  bool fixed;

  FMT_CONSTEXPR digits::result on_digit(char digit, uint64_t divisor,
                                        uint64_t remainder, uint64_t error,
                                        bool integral) {
    FMT_ASSERT(remainder < divisor, "");
    buf[size++] = digit;
    if (!integral && error >= remainder) return digits::error;
    if (size < precision) return digits::more;
    if (!integral) {
      // Check if error * 2 < divisor with overflow prevention.
      // The check is not needed for the integral part because error = 1
      // and divisor > (1 << 32) there.
      if (error >= divisor || error >= divisor - error) return digits::error;
    } else {
      FMT_ASSERT(error == 1 && divisor > 2, "");
    }
    auto dir = get_round_direction(divisor, remainder, error);
    if (dir != round_direction::up)
      return dir == round_direction::down ? digits::done : digits::error;
    ++buf[size - 1];
    for (int i = size - 1; i > 0 && buf[i] > '9'; --i) {
      buf[i] = '0';
      ++buf[i - 1];
    }
    if (buf[0] > '9') {
      buf[0] = '1';
      if (fixed)
        buf[size++] = '0';
      else
        ++exp10;
    }
    return digits::done;
  }
};

// Generates output using the Grisu digit-gen algorithm.
// error: the size of the region (lower, upper) outside of which numbers
// definitely do not round to value (Delta in Grisu3).
FMT_INLINE FMT_CONSTEXPR20 digits::result grisu_gen_digits(
    fp value, uint64_t error, int& exp, gen_digits_handler& handler) {
  const fp one(1ULL << -value.e, value.e);
  // The integral part of scaled value (p1 in Grisu) = value / one. It cannot be
  // zero because it contains a product of two 64-bit numbers with MSB set (due
  // to normalization) - 1, shifted right by at most 60 bits.
  auto integral = static_cast<uint32_t>(value.f >> -one.e);
  FMT_ASSERT(integral != 0, "");
  FMT_ASSERT(integral == value.f >> -one.e, "");
  // The fractional part of scaled value (p2 in Grisu) c = value % one.
  uint64_t fractional = value.f & (one.f - 1);
  exp = count_digits(integral);  // kappa in Grisu.
  // Non-fixed formats require at least one digit and no precision adjustment.
  if (handler.fixed) {
    // Adjust fixed precision by exponent because it is relative to decimal
    // point.
    int precision_offset = exp + handler.exp10;
    if (precision_offset > 0 &&
        handler.precision > max_value<int>() - precision_offset) {
      FMT_THROW(format_error("number is too big"));
    }
    handler.precision += precision_offset;
    // Check if precision is satisfied just by leading zeros, e.g.
    // format("{:.2f}", 0.001) gives "0.00" without generating any digits.
    if (handler.precision <= 0) {
      if (handler.precision < 0) return digits::done;
      // Divide by 10 to prevent overflow.
      uint64_t divisor = impl_data::power_of_10_64[exp - 1] << -one.e;
      auto dir = get_round_direction(divisor, value.f / 10, error * 10);
      if (dir == round_direction::unknown) return digits::error;
      handler.buf[handler.size++] = dir == round_direction::up ? '1' : '0';
      return digits::done;
    }
  }
  // Generate digits for the integral part. This can produce up to 10 digits.
  do {
    uint32_t digit = 0;
    auto divmod_integral = [&](uint32_t divisor) {
      digit = integral / divisor;
      integral %= divisor;
    };
    // This optimization by Milo Yip reduces the number of integer divisions by
    // one per iteration.
    switch (exp) {
    case 10:
      divmod_integral(1000000000);
      break;
    case 9:
      divmod_integral(100000000);
      break;
    case 8:
      divmod_integral(10000000);
      break;
    case 7:
      divmod_integral(1000000);
      break;
    case 6:
      divmod_integral(100000);
      break;
    case 5:
      divmod_integral(10000);
      break;
    case 4:
      divmod_integral(1000);
      break;
    case 3:
      divmod_integral(100);
      break;
    case 2:
      divmod_integral(10);
      break;
    case 1:
      digit = integral;
      integral = 0;
      break;
    default:
      FMT_ASSERT(false, "invalid number of digits");
    }
    --exp;
    auto remainder = (static_cast<uint64_t>(integral) << -one.e) + fractional;
    auto result = handler.on_digit(static_cast<char>('0' + digit),
                                   impl_data::power_of_10_64[exp] << -one.e,
                                   remainder, error, true);
    if (result != digits::more) return result;
  } while (exp > 0);
  // Generate digits for the fractional part.
  for (;;) {
    fractional *= 10;
    error *= 10;
    char digit = static_cast<char>('0' + (fractional >> -one.e));
    fractional &= one.f - 1;
    --exp;
    auto result = handler.on_digit(digit, one.f, fractional, error, false);
    if (result != digits::more) return result;
  }
}

// A 128-bit integer type used internally,
struct uint128_wrapper {
  uint128_wrapper() = default;

#if FMT_USE_INT128
  uint128_t internal_;

  constexpr uint128_wrapper(uint64_t high, uint64_t low) FMT_NOEXCEPT
      : internal_{static_cast<uint128_t>(low) |
                  (static_cast<uint128_t>(high) << 64)} {}

  constexpr uint128_wrapper(uint128_t u) : internal_{u} {}

  constexpr uint64_t high() const FMT_NOEXCEPT {
    return uint64_t(internal_ >> 64);
  }
  constexpr uint64_t low() const FMT_NOEXCEPT { return uint64_t(internal_); }

  uint128_wrapper& operator+=(uint64_t n) FMT_NOEXCEPT {
    internal_ += n;
    return *this;
  }
#else
  uint64_t high_;
  uint64_t low_;

  constexpr uint128_wrapper(uint64_t high, uint64_t low) FMT_NOEXCEPT
      : high_{high},
        low_{low} {}

  constexpr uint64_t high() const FMT_NOEXCEPT { return high_; }
  constexpr uint64_t low() const FMT_NOEXCEPT { return low_; }

  uint128_wrapper& operator+=(uint64_t n) FMT_NOEXCEPT {
#  if defined(_MSC_VER) && defined(_M_X64)
    unsigned char carry = _addcarry_u64(0, low_, n, &low_);
    _addcarry_u64(carry, high_, 0, &high_);
    return *this;
#  else
    uint64_t sum = low_ + n;
    high_ += (sum < low_ ? 1 : 0);
    low_ = sum;
    return *this;
#  endif
  }
#endif
};

// Implementation of Dragonbox algorithm: https://github.com/jk-jeon/dragonbox.
namespace dragonbox {
// Computes 128-bit result of multiplication of two 64-bit unsigned integers.
inline uint128_wrapper umul128(uint64_t x, uint64_t y) FMT_NOEXCEPT {
#if FMT_USE_INT128
  return static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
#elif defined(_MSC_VER) && defined(_M_X64)
  uint128_wrapper result;
  result.low_ = _umul128(x, y, &result.high_);
  return result;
#else
  const uint64_t mask = (uint64_t(1) << 32) - uint64_t(1);

  uint64_t a = x >> 32;
  uint64_t b = x & mask;
  uint64_t c = y >> 32;
  uint64_t d = y & mask;

  uint64_t ac = a * c;
  uint64_t bc = b * c;
  uint64_t ad = a * d;
  uint64_t bd = b * d;

  uint64_t intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

  return {ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
          (intermediate << 32) + (bd & mask)};
#endif
}

// Computes upper 64 bits of multiplication of two 64-bit unsigned integers.
inline uint64_t umul128_upper64(uint64_t x, uint64_t y) FMT_NOEXCEPT {
#if FMT_USE_INT128
  auto p = static_cast<uint128_t>(x) * static_cast<uint128_t>(y);
  return static_cast<uint64_t>(p >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
  return __umulh(x, y);
#else
  return umul128(x, y).high();
#endif
}

// Computes upper 64 bits of multiplication of a 64-bit unsigned integer and a
// 128-bit unsigned integer.
inline uint64_t umul192_upper64(uint64_t x, uint128_wrapper y) FMT_NOEXCEPT {
  uint128_wrapper g0 = umul128(x, y.high());
  g0 += umul128_upper64(x, y.low());
  return g0.high();
}

// Computes upper 32 bits of multiplication of a 32-bit unsigned integer and a
// 64-bit unsigned integer.
inline uint32_t umul96_upper32(uint32_t x, uint64_t y) FMT_NOEXCEPT {
  return static_cast<uint32_t>(umul128_upper64(x, y));
}

// Computes middle 64 bits of multiplication of a 64-bit unsigned integer and a
// 128-bit unsigned integer.
inline uint64_t umul192_middle64(uint64_t x, uint128_wrapper y) FMT_NOEXCEPT {
  uint64_t g01 = x * y.high();
  uint64_t g10 = umul128_upper64(x, y.low());
  return g01 + g10;
}

// Computes lower 64 bits of multiplication of a 32-bit unsigned integer and a
// 64-bit unsigned integer.
inline uint64_t umul96_lower64(uint32_t x, uint64_t y) FMT_NOEXCEPT {
  return x * y;
}

// Computes floor(log10(pow(2, e))) for e in [-1700, 1700] using the method from
// https://fmt.dev/papers/Grisu-Exact.pdf#page=5, section 3.4.
inline int floor_log10_pow2(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1700 && e >= -1700, "too large exponent");
  const int shift = 22;
  return (e * static_cast<int>(log10_2_significand >> (64 - shift))) >> shift;
}

// Various fast log computations.
inline int floor_log2_pow10(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1233 && e >= -1233, "too large exponent");
  const uint64_t log2_10_integer_part = 3;
  const uint64_t log2_10_fractional_digits = 0x5269e12f346e2bf9;
  const int shift_amount = 19;
  return (e * static_cast<int>(
                  (log2_10_integer_part << shift_amount) |
                  (log2_10_fractional_digits >> (64 - shift_amount)))) >>
         shift_amount;
}
inline int floor_log10_pow2_minus_log10_4_over_3(int e) FMT_NOEXCEPT {
  FMT_ASSERT(e <= 1700 && e >= -1700, "too large exponent");
  const uint64_t log10_4_over_3_fractional_digits = 0x1ffbfc2bbc780375;
  const int shift_amount = 22;
  return (e * static_cast<int>(log10_2_significand >> (64 - shift_amount)) -
          static_cast<int>(log10_4_over_3_fractional_digits >>
                           (64 - shift_amount))) >>
         shift_amount;
}

// Returns true iff x is divisible by pow(2, exp).
inline bool divisible_by_power_of_2(uint32_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp >= 1, "");
  FMT_ASSERT(x != 0, "");
#ifdef FMT_BUILTIN_CTZ
  return FMT_BUILTIN_CTZ(x) >= exp;
#else
  return exp < num_bits<uint32_t>() && x == ((x >> exp) << exp);
#endif
}
inline bool divisible_by_power_of_2(uint64_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp >= 1, "");
  FMT_ASSERT(x != 0, "");
#ifdef FMT_BUILTIN_CTZLL
  return FMT_BUILTIN_CTZLL(x) >= exp;
#else
  return exp < num_bits<uint64_t>() && x == ((x >> exp) << exp);
#endif
}

// Table entry type for divisibility test.
template <typename T> struct divtest_table_entry {
  T mod_inv;
  T max_quotient;
};

// Returns true iff x is divisible by pow(5, exp).
inline bool divisible_by_power_of_5(uint32_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp <= 10, "too large exponent");
  static constexpr const divtest_table_entry<uint32_t> divtest_table[] = {
      {0x00000001, 0xffffffff}, {0xcccccccd, 0x33333333},
      {0xc28f5c29, 0x0a3d70a3}, {0x26e978d5, 0x020c49ba},
      {0x3afb7e91, 0x0068db8b}, {0x0bcbe61d, 0x0014f8b5},
      {0x68c26139, 0x000431bd}, {0xae8d46a5, 0x0000d6bf},
      {0x22e90e21, 0x00002af3}, {0x3a2e9c6d, 0x00000897},
      {0x3ed61f49, 0x000001b7}};
  return x * divtest_table[exp].mod_inv <= divtest_table[exp].max_quotient;
}
inline bool divisible_by_power_of_5(uint64_t x, int exp) FMT_NOEXCEPT {
  FMT_ASSERT(exp <= 23, "too large exponent");
  static constexpr const divtest_table_entry<uint64_t> divtest_table[] = {
      {0x0000000000000001, 0xffffffffffffffff},
      {0xcccccccccccccccd, 0x3333333333333333},
      {0x8f5c28f5c28f5c29, 0x0a3d70a3d70a3d70},
      {0x1cac083126e978d5, 0x020c49ba5e353f7c},
      {0xd288ce703afb7e91, 0x0068db8bac710cb2},
      {0x5d4e8fb00bcbe61d, 0x0014f8b588e368f0},
      {0x790fb65668c26139, 0x000431bde82d7b63},
      {0xe5032477ae8d46a5, 0x0000d6bf94d5e57a},
      {0xc767074b22e90e21, 0x00002af31dc46118},
      {0x8e47ce423a2e9c6d, 0x0000089705f4136b},
      {0x4fa7f60d3ed61f49, 0x000001b7cdfd9d7b},
      {0x0fee64690c913975, 0x00000057f5ff85e5},
      {0x3662e0e1cf503eb1, 0x000000119799812d},
      {0xa47a2cf9f6433fbd, 0x0000000384b84d09},
      {0x54186f653140a659, 0x00000000b424dc35},
      {0x7738164770402145, 0x0000000024075f3d},
      {0xe4a4d1417cd9a041, 0x000000000734aca5},
      {0xc75429d9e5c5200d, 0x000000000170ef54},
      {0xc1773b91fac10669, 0x000000000049c977},
      {0x26b172506559ce15, 0x00000000000ec1e4},
      {0xd489e3a9addec2d1, 0x000000000002f394},
      {0x90e860bb892c8d5d, 0x000000000000971d},
      {0x502e79bf1b6f4f79, 0x0000000000001e39},
      {0xdcd618596be30fe5, 0x000000000000060b}};
  return x * divtest_table[exp].mod_inv <= divtest_table[exp].max_quotient;
}

// Replaces n by floor(n / pow(5, N)) returning true if and only if n is
// divisible by pow(5, N).
// Precondition: n <= 2 * pow(5, N + 1).
template <int N>
bool check_divisibility_and_divide_by_pow5(uint32_t& n) FMT_NOEXCEPT {
  static constexpr struct {
    uint32_t magic_number;
    int bits_for_comparison;
    uint32_t threshold;
    int shift_amount;
  } infos[] = {{0xcccd, 16, 0x3333, 18}, {0xa429, 8, 0x0a, 20}};
  constexpr auto info = infos[N - 1];
  n *= info.magic_number;
  const uint32_t comparison_mask = (1u << info.bits_for_comparison) - 1;
  bool result = (n & comparison_mask) <= info.threshold;
  n >>= info.shift_amount;
  return result;
}

// Computes floor(n / pow(10, N)) for small n and N.
// Precondition: n <= pow(10, N + 1).
template <int N> uint32_t small_division_by_pow10(uint32_t n) FMT_NOEXCEPT {
  static constexpr struct {
    uint32_t magic_number;
    int shift_amount;
    uint32_t divisor_times_10;
  } infos[] = {{0xcccd, 19, 100}, {0xa3d8, 22, 1000}};
  constexpr auto info = infos[N - 1];
  FMT_ASSERT(n <= info.divisor_times_10, "n is too large");
  return n * info.magic_number >> info.shift_amount;
}

// Computes floor(n / 10^(kappa + 1)) (float)
inline uint32_t divide_by_10_to_kappa_plus_1(uint32_t n) FMT_NOEXCEPT {
  return n / float_info<float>::big_divisor;
}
// Computes floor(n / 10^(kappa + 1)) (double)
inline uint64_t divide_by_10_to_kappa_plus_1(uint64_t n) FMT_NOEXCEPT {
  return umul128_upper64(n, 0x83126e978d4fdf3c) >> 9;
}

// Various subroutines using pow10 cache
template <class T> struct cache_accessor;

template <> struct cache_accessor<float> {
  using carrier_uint = float_info<float>::carrier_uint;
  using cache_entry_type = uint64_t;

  static uint64_t get_cached_power(int k) FMT_NOEXCEPT {
    FMT_ASSERT(k >= float_info<float>::min_k && k <= float_info<float>::max_k,
               "k is out of range");
    static constexpr const uint64_t pow10_significands[] = {
        0x81ceb32c4b43fcf5, 0xa2425ff75e14fc32, 0xcad2f7f5359a3b3f,
        0xfd87b5f28300ca0e, 0x9e74d1b791e07e49, 0xc612062576589ddb,
        0xf79687aed3eec552, 0x9abe14cd44753b53, 0xc16d9a0095928a28,
        0xf1c90080baf72cb2, 0x971da05074da7bef, 0xbce5086492111aeb,
        0xec1e4a7db69561a6, 0x9392ee8e921d5d08, 0xb877aa3236a4b44a,
        0xe69594bec44de15c, 0x901d7cf73ab0acda, 0xb424dc35095cd810,
        0xe12e13424bb40e14, 0x8cbccc096f5088cc, 0xafebff0bcb24aaff,
        0xdbe6fecebdedd5bf, 0x89705f4136b4a598, 0xabcc77118461cefd,
        0xd6bf94d5e57a42bd, 0x8637bd05af6c69b6, 0xa7c5ac471b478424,
        0xd1b71758e219652c, 0x83126e978d4fdf3c, 0xa3d70a3d70a3d70b,
        0xcccccccccccccccd, 0x8000000000000000, 0xa000000000000000,
        0xc800000000000000, 0xfa00000000000000, 0x9c40000000000000,
        0xc350000000000000, 0xf424000000000000, 0x9896800000000000,
        0xbebc200000000000, 0xee6b280000000000, 0x9502f90000000000,
        0xba43b74000000000, 0xe8d4a51000000000, 0x9184e72a00000000,
        0xb5e620f480000000, 0xe35fa931a0000000, 0x8e1bc9bf04000000,
        0xb1a2bc2ec5000000, 0xde0b6b3a76400000, 0x8ac7230489e80000,
        0xad78ebc5ac620000, 0xd8d726b7177a8000, 0x878678326eac9000,
        0xa968163f0a57b400, 0xd3c21bcecceda100, 0x84595161401484a0,
        0xa56fa5b99019a5c8, 0xcecb8f27f4200f3a, 0x813f3978f8940984,
        0xa18f07d736b90be5, 0xc9f2c9cd04674ede, 0xfc6f7c4045812296,
        0x9dc5ada82b70b59d, 0xc5371912364ce305, 0xf684df56c3e01bc6,
        0x9a130b963a6c115c, 0xc097ce7bc90715b3, 0xf0bdc21abb48db20,
        0x96769950b50d88f4, 0xbc143fa4e250eb31, 0xeb194f8e1ae525fd,
        0x92efd1b8d0cf37be, 0xb7abc627050305ad, 0xe596b7b0c643c719,
        0x8f7e32ce7bea5c6f, 0xb35dbf821ae4f38b, 0xe0352f62a19e306e};
    return pow10_significands[k - float_info<float>::min_k];
  }

  static carrier_uint compute_mul(carrier_uint u,
                                  const cache_entry_type& cache) FMT_NOEXCEPT {
    return umul96_upper32(u, cache);
  }

  static uint32_t compute_delta(const cache_entry_type& cache,
                                int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<uint32_t>(cache >> (64 - 1 - beta_minus_1));
  }

  static bool compute_mul_parity(carrier_uint two_f,
                                 const cache_entry_type& cache,
                                 int beta_minus_1) FMT_NOEXCEPT {
    FMT_ASSERT(beta_minus_1 >= 1, "");
    FMT_ASSERT(beta_minus_1 < 64, "");

    return ((umul96_lower64(two_f, cache) >> (64 - beta_minus_1)) & 1) != 0;
  }

  static carrier_uint compute_left_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<carrier_uint>(
        (cache - (cache >> (float_info<float>::significand_bits + 2))) >>
        (64 - float_info<float>::significand_bits - 1 - beta_minus_1));
  }

  static carrier_uint compute_right_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<carrier_uint>(
        (cache + (cache >> (float_info<float>::significand_bits + 1))) >>
        (64 - float_info<float>::significand_bits - 1 - beta_minus_1));
  }

  static carrier_uint compute_round_up_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (static_cast<carrier_uint>(
                cache >>
                (64 - float_info<float>::significand_bits - 2 - beta_minus_1)) +
            1) /
           2;
  }
};

template <> struct cache_accessor<double> {
  using carrier_uint = float_info<double>::carrier_uint;
  using cache_entry_type = uint128_wrapper;

  static uint128_wrapper get_cached_power(int k) FMT_NOEXCEPT {
    FMT_ASSERT(k >= float_info<double>::min_k && k <= float_info<double>::max_k,
               "k is out of range");

    static constexpr const uint128_wrapper pow10_significands[] = {
#if FMT_USE_FULL_CACHE_DRAGONBOX
      {0xff77b1fcbebcdc4f, 0x25e8e89c13bb0f7b},
      {0x9faacf3df73609b1, 0x77b191618c54e9ad},
      {0xc795830d75038c1d, 0xd59df5b9ef6a2418},
      {0xf97ae3d0d2446f25, 0x4b0573286b44ad1e},
      {0x9becce62836ac577, 0x4ee367f9430aec33},
      {0xc2e801fb244576d5, 0x229c41f793cda740},
      {0xf3a20279ed56d48a, 0x6b43527578c11110},
      {0x9845418c345644d6, 0x830a13896b78aaaa},
      {0xbe5691ef416bd60c, 0x23cc986bc656d554},
      {0xedec366b11c6cb8f, 0x2cbfbe86b7ec8aa9},
      {0x94b3a202eb1c3f39, 0x7bf7d71432f3d6aa},
      {0xb9e08a83a5e34f07, 0xdaf5ccd93fb0cc54},
      {0xe858ad248f5c22c9, 0xd1b3400f8f9cff69},
      {0x91376c36d99995be, 0x23100809b9c21fa2},
      {0xb58547448ffffb2d, 0xabd40a0c2832a78b},
      {0xe2e69915b3fff9f9, 0x16c90c8f323f516d},
      {0x8dd01fad907ffc3b, 0xae3da7d97f6792e4},
      {0xb1442798f49ffb4a, 0x99cd11cfdf41779d},
      {0xdd95317f31c7fa1d, 0x40405643d711d584},
      {0x8a7d3eef7f1cfc52, 0x482835ea666b2573},
      {0xad1c8eab5ee43b66, 0xda3243650005eed0},
      {0xd863b256369d4a40, 0x90bed43e40076a83},
      {0x873e4f75e2224e68, 0x5a7744a6e804a292},
      {0xa90de3535aaae202, 0x711515d0a205cb37},
      {0xd3515c2831559a83, 0x0d5a5b44ca873e04},
      {0x8412d9991ed58091, 0xe858790afe9486c3},
      {0xa5178fff668ae0b6, 0x626e974dbe39a873},
      {0xce5d73ff402d98e3, 0xfb0a3d212dc81290},
      {0x80fa687f881c7f8e, 0x7ce66634bc9d0b9a},
      {0xa139029f6a239f72, 0x1c1fffc1ebc44e81},
      {0xc987434744ac874e, 0xa327ffb266b56221},
      {0xfbe9141915d7a922, 0x4bf1ff9f0062baa9},
      {0x9d71ac8fada6c9b5, 0x6f773fc3603db4aa},
      {0xc4ce17b399107c22, 0xcb550fb4384d21d4},
      {0xf6019da07f549b2b, 0x7e2a53a146606a49},
      {0x99c102844f94e0fb, 0x2eda7444cbfc426e},
      {0xc0314325637a1939, 0xfa911155fefb5309},
      {0xf03d93eebc589f88, 0x793555ab7eba27cb},
      {0x96267c7535b763b5, 0x4bc1558b2f3458df},
      {0xbbb01b9283253ca2, 0x9eb1aaedfb016f17},
      {0xea9c227723ee8bcb, 0x465e15a979c1cadd},
      {0x92a1958a7675175f, 0x0bfacd89ec191eca},
      {0xb749faed14125d36, 0xcef980ec671f667c},
      {0xe51c79a85916f484, 0x82b7e12780e7401b},
      {0x8f31cc0937ae58d2, 0xd1b2ecb8bf0f156b},
      {0xb2fe3f0b8599ef07, 0x861fa7e6dcb4aa16},
      {0xdfbdcece67006ac9, 0x67a791e093e1d49b},
      {0x8bd6a141006042bd, 0xe0c8bb2c5c6d24e1},
      {0xaecc49914078536d, 0x58fae9f773886e19},
      {0xda7f5bf590966848, 0xaf39a475506a899f},
      {0x888f99797a5e012d, 0x6d8406c952429604},
      {0xaab37fd7d8f58178, 0xc8e5087ba6d33b84},
      {0xd5605fcdcf32e1d6, 0xfb1e4a9a90880a65},
      {0x855c3be0a17fcd26, 0x5cf2eea09a550680},
      {0xa6b34ad8c9dfc06f, 0xf42faa48c0ea481f},
      {0xd0601d8efc57b08b, 0xf13b94daf124da27},
      {0x823c12795db6ce57, 0x76c53d08d6b70859},
      {0xa2cb1717b52481ed, 0x54768c4b0c64ca6f},
      {0xcb7ddcdda26da268, 0xa9942f5dcf7dfd0a},
      {0xfe5d54150b090b02, 0xd3f93b35435d7c4d},
      {0x9efa548d26e5a6e1, 0xc47bc5014a1a6db0},
      {0xc6b8e9b0709f109a, 0x359ab6419ca1091c},
      {0xf867241c8cc6d4c0, 0xc30163d203c94b63},
      {0x9b407691d7fc44f8, 0x79e0de63425dcf1e},
      {0xc21094364dfb5636, 0x985915fc12f542e5},
      {0xf294b943e17a2bc4, 0x3e6f5b7b17b2939e},
      {0x979cf3ca6cec5b5a, 0xa705992ceecf9c43},
      {0xbd8430bd08277231, 0x50c6ff782a838354},
      {0xece53cec4a314ebd, 0xa4f8bf5635246429},
      {0x940f4613ae5ed136, 0x871b7795e136be9a},
      {0xb913179899f68584, 0x28e2557b59846e40},
      {0xe757dd7ec07426e5, 0x331aeada2fe589d0},
      {0x9096ea6f3848984f, 0x3ff0d2c85def7622},
      {0xb4bca50b065abe63, 0x0fed077a756b53aa},
      {0xe1ebce4dc7f16dfb, 0xd3e8495912c62895},
      {0x8d3360f09cf6e4bd, 0x64712dd7abbbd95d},
      {0xb080392cc4349dec, 0xbd8d794d96aacfb4},
      {0xdca04777f541c567, 0xecf0d7a0fc5583a1},
      {0x89e42caaf9491b60, 0xf41686c49db57245},
      {0xac5d37d5b79b6239, 0x311c2875c522ced6},
      {0xd77485cb25823ac7, 0x7d633293366b828c},
      {0x86a8d39ef77164bc, 0xae5dff9c02033198},
      {0xa8530886b54dbdeb, 0xd9f57f830283fdfd},
      {0xd267caa862a12d66, 0xd072df63c324fd7c},
      {0x8380dea93da4bc60, 0x4247cb9e59f71e6e},
      {0xa46116538d0deb78, 0x52d9be85f074e609},
      {0xcd795be870516656, 0x67902e276c921f8c},
      {0x806bd9714632dff6, 0x00ba1cd8a3db53b7},
      {0xa086cfcd97bf97f3, 0x80e8a40eccd228a5},
      {0xc8a883c0fdaf7df0, 0x6122cd128006b2ce},
      {0xfad2a4b13d1b5d6c, 0x796b805720085f82},
      {0x9cc3a6eec6311a63, 0xcbe3303674053bb1},
      {0xc3f490aa77bd60fc, 0xbedbfc4411068a9d},
      {0xf4f1b4d515acb93b, 0xee92fb5515482d45},
      {0x991711052d8bf3c5, 0x751bdd152d4d1c4b},
      {0xbf5cd54678eef0b6, 0xd262d45a78a0635e},
      {0xef340a98172aace4, 0x86fb897116c87c35},
      {0x9580869f0e7aac0e, 0xd45d35e6ae3d4da1},
      {0xbae0a846d2195712, 0x8974836059cca10a},
      {0xe998d258869facd7, 0x2bd1a438703fc94c},
      {0x91ff83775423cc06, 0x7b6306a34627ddd0},
      {0xb67f6455292cbf08, 0x1a3bc84c17b1d543},
      {0xe41f3d6a7377eeca, 0x20caba5f1d9e4a94},
      {0x8e938662882af53e, 0x547eb47b7282ee9d},
      {0xb23867fb2a35b28d, 0xe99e619a4f23aa44},
      {0xdec681f9f4c31f31, 0x6405fa00e2ec94d5},
      {0x8b3c113c38f9f37e, 0xde83bc408dd3dd05},
      {0xae0b158b4738705e, 0x9624ab50b148d446},
      {0xd98ddaee19068c76, 0x3badd624dd9b0958},
      {0x87f8a8d4cfa417c9, 0xe54ca5d70a80e5d7},
      {0xa9f6d30a038d1dbc, 0x5e9fcf4ccd211f4d},
      {0xd47485cb25823ac7, 0x7d633293366b828c},
      {0x84c8d4dfd2c63f3b, 0x29ecd9f402033198},
      {0xa5fb0a17c777cf09, 0xf468107100525891},
      {0xcf79cc9db955c2cc, 0x7182148d4066eeb5},
      {0x81ac1fe293d599bf, 0xc6f14cd848405531},
      {0xa21727db38cb002f, 0xb8ada00e5a506a7d},
      {0xca9cf1d206fdc03b, 0xa6d90811f0e4851d},
      {0xfd442e4688bd304a, 0x908f4a166d1da664},
      {0x9e4a9cec15763e2e, 0x9a598e4e043287ff},
      {0xc5dd44271ad3cdba, 0x40eff1e1853f29fe},
      {0xf7549530e188c128, 0xd12bee59e68ef47d},
      {0x9a94dd3e8cf578b9, 0x82bb74f8301958cf},
      {0xc13a148e3032d6e7, 0xe36a52363c1faf02},
      {0xf18899b1bc3f8ca1, 0xdc44e6c3cb279ac2},
      {0x96f5600f15a7b7e5, 0x29ab103a5ef8c0ba},
      {0xbcb2b812db11a5de, 0x7415d448f6b6f0e8},
      {0xebdf661791d60f56, 0x111b495b3464ad22},
      {0x936b9fcebb25c995, 0xcab10dd900beec35},
      {0xb84687c269ef3bfb, 0x3d5d514f40eea743},
      {0xe65829b3046b0afa, 0x0cb4a5a3112a5113},
      {0x8ff71a0fe2c6e6dc, 0x47f0e785eaba72ac},
      {0xb3f4e093db73a093, 0x59ed216765690f57},
      {0xe0f218b8d25088b8, 0x306869c13ec3532d},
      {0x8c974f7383725573, 0x1e414218c73a13fc},
      {0xafbd2350644eeacf, 0xe5d1929ef90898fb},
      {0xdbac6c247d62a583, 0xdf45f746b74abf3a},
      {0x894bc396ce5da772, 0x6b8bba8c328eb784},
      {0xab9eb47c81f5114f, 0x066ea92f3f326565},
      {0xd686619ba27255a2, 0xc80a537b0efefebe},
      {0x8613fd0145877585, 0xbd06742ce95f5f37},
      {0xa798fc4196e952e7, 0x2c48113823b73705},
      {0xd17f3b51fca3a7a0, 0xf75a15862ca504c6},
      {0x82ef85133de648c4, 0x9a984d73dbe722fc},
      {0xa3ab66580d5fdaf5, 0xc13e60d0d2e0ebbb},
      {0xcc963fee10b7d1b3, 0x318df905079926a9},
      {0xffbbcfe994e5c61f, 0xfdf17746497f7053},
      {0x9fd561f1fd0f9bd3, 0xfeb6ea8bedefa634},
      {0xc7caba6e7c5382c8, 0xfe64a52ee96b8fc1},
      {0xf9bd690a1b68637b, 0x3dfdce7aa3c673b1},
      {0x9c1661a651213e2d, 0x06bea10ca65c084f},
      {0xc31bfa0fe5698db8, 0x486e494fcff30a63},
      {0xf3e2f893dec3f126, 0x5a89dba3c3efccfb},
      {0x986ddb5c6b3a76b7, 0xf89629465a75e01d},
      {0xbe89523386091465, 0xf6bbb397f1135824},
      {0xee2ba6c0678b597f, 0x746aa07ded582e2d},
      {0x94db483840b717ef, 0xa8c2a44eb4571cdd},
      {0xba121a4650e4ddeb, 0x92f34d62616ce414},
      {0xe896a0d7e51e1566, 0x77b020baf9c81d18},
      {0x915e2486ef32cd60, 0x0ace1474dc1d122f},
      {0xb5b5ada8aaff80b8, 0x0d819992132456bb},
      {0xe3231912d5bf60e6, 0x10e1fff697ed6c6a},
      {0x8df5efabc5979c8f, 0xca8d3ffa1ef463c2},
      {0xb1736b96b6fd83b3, 0xbd308ff8a6b17cb3},
      {0xddd0467c64bce4a0, 0xac7cb3f6d05ddbdf},
      {0x8aa22c0dbef60ee4, 0x6bcdf07a423aa96c},
      {0xad4ab7112eb3929d, 0x86c16c98d2c953c7},
      {0xd89d64d57a607744, 0xe871c7bf077ba8b8},
      {0x87625f056c7c4a8b, 0x11471cd764ad4973},
      {0xa93af6c6c79b5d2d, 0xd598e40d3dd89bd0},
      {0xd389b47879823479, 0x4aff1d108d4ec2c4},
      {0x843610cb4bf160cb, 0xcedf722b5e62a7ac9000},
      {0xa54394fe1eedb8fe, 0xc2974eb4ee6589dd},
      {0xce947a3da6a9273e, 0x733d226229feea33},
      {0x811ccc668829b887, 0x0806357d5a3f5260},
      {0xa163ff802a3426a8, 0xca07c2dcb0cf26f8},
      {0xc9bcff6034c13052, 0xfc89b393dd02f0b6},
      {0xfc2c3f3841f17c67, 0xbbac2078d443ace3},
      {0x9d9ba7832936edc0, 0xd54b944b84aa4c0e},
      {0xc5029163f384a931, 0x0a9e795e65d4df12},
      {0xf64335bcf065d37d, 0x4d4617b5ff4a16d6},
      {0x99ea0196163fa42e, 0x504bced1bf8e4e46},
      {0xc06481fb9bcf8d39, 0xe45ec2862f71e1d7},
      {0xf07da27a82c37088, 0x5d767327bb4e5a4d},
      {0x964e858c91ba2655, 0x3a6a07f8d510f870},
      {0xbbe226efb628afea, 0x890489f70a55368c},
      {0xeadab0aba3b2dbe5, 0x2b45ac74ccea842f},
      {0x92c8ae6b464fc96f, 0x3b0b8bc90012929e},
      {0xb77ada0617e3bbcb, 0x09ce6ebb40173745},
      {0xe55990879ddcaabd, 0xcc420a6a101d0516},
      {0x8f57fa54c2a9eab6, 0x9fa946824a12232e},
      {0xb32df8e9f3546564, 0x47939822dc96abfa},
      {0xdff9772470297ebd, 0x59787e2b93bc56f8},
      {0x8bfbea76c619ef36, 0x57eb4edb3c55b65b},
      {0xaefae51477a06b03, 0xede622920b6b23f2},
      {0xdab99e59958885c4, 0xe95fab368e45ecee},
      {0x88b402f7fd75539b, 0x11dbcb0218ebb415},
      {0xaa242499697392d2, 0xdde50bd1d5d0b9ea},
      {0xd4ad2dbfc3d07787, 0x955e4ec64b44e865}};

#if FMT_USE_FULL_CACHE_DRAGONBOX
    return pow10_significands[k - float_info<double>::min_k];
#else
    static constexpr const uint64_t powers_of_5_64[] = {
        0x0000000000000001, 0x0000000000000005, 0x0000000000000019,
        0x000000000000007d, 0x0000000000000271, 0x0000000000000c35,
        0x0000000000003d09, 0x000000000001312d, 0x000000000005f5e1,
        0x00000000001dcd65, 0x00000000009502f9, 0x0000000002e90edd,
        0x000000000e8d4a51, 0x0000000048c27395, 0x000000016bcc41e9,
        0x000000071afd498d, 0x0000002386f26fc1, 0x000000b1a2bc2ec5,
        0x000003782dace9d9, 0x00001158e460913d, 0x000056bc75e2d631,
        0x0001b1ae4d6e2ef5, 0x000878678326eac9, 0x002a5a058fc295ed,
        0x00d3c21bcecceda1, 0x0422ca8b0a00a425, 0x14adf4b7320334b9};

    static constexpr const uint32_t pow10_recovery_errors[] = {
        0x50001400, 0x54044100, 0x54014555, 0x55954415, 0x54115555, 0x00000001,
        0x50000000, 0x00104000, 0x54010004, 0x05004001, 0x55555444, 0x41545555,
        0x54040551, 0x15445545, 0x51555514, 0x10000015, 0x00101100, 0x01100015,
        0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x04450514, 0x45414110,
        0x55555145, 0x50544050, 0x15040155, 0x11054140, 0x50111514, 0x11451454,
        0x00400541, 0x00000000, 0x55555450, 0x10056551, 0x10054011, 0x55551014,
        0x69514555, 0x05151109, 0x00155555};

    static const int compression_ratio = 27;

    // Compute base index.
    int cache_index = (k - float_info<double>::min_k) / compression_ratio;
    int kb = cache_index * compression_ratio + float_info<double>::min_k;
    int offset = k - kb;

    // Get base cache.
    uint128_wrapper base_cache = pow10_significands[cache_index];
    if (offset == 0) return base_cache;

    // Compute the required amount of bit-shift.
    int alpha = floor_log2_pow10(kb + offset) - floor_log2_pow10(kb) - offset;
    FMT_ASSERT(alpha > 0 && alpha < 64, "shifting error detected");

    // Try to recover the real cache.
    uint64_t pow5 = powers_of_5_64[offset];
    uint128_wrapper recovered_cache = umul128(base_cache.high(), pow5);
    uint128_wrapper middle_low =
        umul128(base_cache.low() - (kb < 0 ? 1u : 0u), pow5);

    recovered_cache += middle_low.high();

    uint64_t high_to_middle = recovered_cache.high() << (64 - alpha);
    uint64_t middle_to_low = recovered_cache.low() << (64 - alpha);

    recovered_cache =
        uint128_wrapper{(recovered_cache.low() >> alpha) | high_to_middle,
                        ((middle_low.low() >> alpha) | middle_to_low)};

    if (kb < 0) recovered_cache += 1;

    // Get error.
    int error_idx = (k - float_info<double>::min_k) / 16;
    uint32_t error = (pow10_recovery_errors[error_idx] >>
                      ((k - float_info<double>::min_k) % 16) * 2) &
                     0x3;

    // Add the error back.
    FMT_ASSERT(recovered_cache.low() + error >= recovered_cache.low(), "");
    return {recovered_cache.high(), recovered_cache.low() + error};
#endif
  }

  static carrier_uint compute_mul(carrier_uint u,
                                  const cache_entry_type& cache) FMT_NOEXCEPT {
    return umul192_upper64(u, cache);
  }

  static uint32_t compute_delta(cache_entry_type const& cache,
                                int beta_minus_1) FMT_NOEXCEPT {
    return static_cast<uint32_t>(cache.high() >> (64 - 1 - beta_minus_1));
  }

  static bool compute_mul_parity(carrier_uint two_f,
                                 const cache_entry_type& cache,
                                 int beta_minus_1) FMT_NOEXCEPT {
    FMT_ASSERT(beta_minus_1 >= 1, "");
    FMT_ASSERT(beta_minus_1 < 64, "");

    return ((umul192_middle64(two_f, cache) >> (64 - beta_minus_1)) & 1) != 0;
  }

  static carrier_uint compute_left_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (cache.high() -
            (cache.high() >> (float_info<double>::significand_bits + 2))) >>
           (64 - float_info<double>::significand_bits - 1 - beta_minus_1);
  }

  static carrier_uint compute_right_endpoint_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return (cache.high() +
            (cache.high() >> (float_info<double>::significand_bits + 1))) >>
           (64 - float_info<double>::significand_bits - 1 - beta_minus_1);
  }

  static carrier_uint compute_round_up_for_shorter_interval_case(
      const cache_entry_type& cache, int beta_minus_1) FMT_NOEXCEPT {
    return ((cache.high() >>
             (64 - float_info<double>::significand_bits - 2 - beta_minus_1)) +
            1) /
           2;
  }
};

// Various integer checks
template <class T>
bool is_left_endpoint_integer_shorter_interval(int exponent) FMT_NOEXCEPT {
  return exponent >=
             float_info<
                 T>::case_shorter_interval_left_endpoint_lower_threshold &&
         exponent <=
             float_info<T>::case_shorter_interval_left_endpoint_upper_threshold;
}
template <class T>
bool is_endpoint_integer(typename float_info<T>::carrier_uint two_f,
                         int exponent, int minus_k) FMT_NOEXCEPT {
  if (exponent < float_info<T>::case_fc_pm_half_lower_threshold) return false;
  // For k >= 0.
  if (exponent <= float_info<T>::case_fc_pm_half_upper_threshold) return true;
  // For k < 0.
  if (exponent > float_info<T>::divisibility_check_by_5_threshold) return false;
  return divisible_by_power_of_5(two_f, minus_k);
}

template <class T>
bool is_center_integer(typename float_info<T>::carrier_uint two_f, int exponent,
                       int minus_k) FMT_NOEXCEPT {
  // Exponent for 5 is negative.
  if (exponent > float_info<T>::divisibility_check_by_5_threshold) return false;
  if (exponent > float_info<T>::case_fc_upper_threshold)
    return divisible_by_power_of_5(two_f, minus_k);
  // Both exponents are nonnegative.
  if (exponent >= float_info<T>::case_fc_lower_threshold) return true;
  // Exponent for 2 is negative.
  return divisible_by_power_of_2(two_f, minus_k - exponent + 1);
}

// Remove trailing zeros from n and return the number of zeros removed (float)
FMT_INLINE int remove_trailing_zeros(uint32_t& n) FMT_NOEXCEPT {
#ifdef FMT_BUILTIN_CTZ
  int t = FMT_BUILTIN_CTZ(n);
#else
  int t = ctz(n);
#endif
  if (t > float_info<float>::max_trailing_zeros)
    t = float_info<float>::max_trailing_zeros;

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint32_t mod_inv2 = 0xc28f5c29;
  const uint32_t max_quotient2 = 0x0a3d70a3;

  int s = 0;
  for (; s < t - 1; s += 2) {
    if (n * mod_inv2 > max_quotient2) break;
    n *= mod_inv2;
  }
  if (s < t && n * mod_inv1 <= max_quotient1) {
    n *= mod_inv1;
    ++s;
  }
  n >>= s;
  return s;
}

// Removes trailing zeros and returns the number of zeros removed (double)
FMT_INLINE int remove_trailing_zeros(uint64_t& n) FMT_NOEXCEPT {
#ifdef FMT_BUILTIN_CTZLL
  int t = FMT_BUILTIN_CTZLL(n);
#else
  int t = ctzll(n);
#endif
  if (t > float_info<double>::max_trailing_zeros)
    t = float_info<double>::max_trailing_zeros;
  // Divide by 10^8 and reduce to 32-bits
  // Since ret_value.significand <= (2^64 - 1) / 1000 < 10^17,
  // both of the quotient and the r should fit in 32-bits

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint64_t mod_inv8 = 0xc767074b22e90e21;
  const uint64_t max_quotient8 = 0x00002af31dc46118;

  // If the number is divisible by 1'0000'0000, work with the quotient
  if (t >= 8) {
    auto quotient_candidate = n * mod_inv8;

    if (quotient_candidate <= max_quotient8) {
      auto quotient = static_cast<uint32_t>(quotient_candidate >> 8);

      int s = 8;
      for (; s < t; ++s) {
        if (quotient * mod_inv1 > max_quotient1) break;
        quotient *= mod_inv1;
      }
      quotient >>= (s - 8);
      n = quotient;
      return s;
    }
  }

  // Otherwise, work with the remainder
  auto quotient = static_cast<uint32_t>(n / 100000000);
  auto remainder = static_cast<uint32_t>(n - 100000000 * quotient);

  if (t == 0 || remainder * mod_inv1 > max_quotient1) {
    return 0;
  }
  remainder *= mod_inv1;

  if (t == 1 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 1) + quotient * 10000000ull;
    return 1;
  }
  remainder *= mod_inv1;

  if (t == 2 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 2) + quotient * 1000000ull;
    return 2;
  }
  remainder *= mod_inv1;

  if (t == 3 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 3) + quotient * 100000ull;
    return 3;
  }
  remainder *= mod_inv1;

  if (t == 4 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 4) + quotient * 10000ull;
    return 4;
  }
  remainder *= mod_inv1;

  if (t == 5 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 5) + quotient * 1000ull;
    return 5;
  }
  remainder *= mod_inv1;

  if (t == 6 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 6) + quotient * 100ull;
    return 6;
  }
  remainder *= mod_inv1;

  n = (remainder >> 7) + quotient * 10ull;
  return 7;
}

// The main algorithm for shorter interval case
template <class T>
FMT_INLINE decimal_fp<T> shorter_interval_case(int exponent) FMT_NOEXCEPT {
  decimal_fp<T> ret_value;
  // Compute k and beta
  const int minus_k = floor_log10_pow2_minus_log10_4_over_3(exponent);
  const int beta_minus_1 = exponent + floor_log2_pow10(-minus_k);

  // Compute xi and zi
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);

  auto xi = cache_accessor<T>::compute_left_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);
  auto zi = cache_accessor<T>::compute_right_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);

  // If the left endpoint is not an integer, increase it
  if (!is_left_endpoint_integer_shorter_interval<T>(exponent)) ++xi;

  // Try bigger divisor
  ret_value.significand = zi / 10;

  // If succeed, remove trailing zeros if necessary and return
  if (ret_value.significand * 10 >= xi) {
    ret_value.exponent = minus_k + 1;
    ret_value.exponent += remove_trailing_zeros(ret_value.significand);
    return ret_value;
  }

  // Otherwise, compute the round-up of y
  ret_value.significand =
      cache_accessor<T>::compute_round_up_for_shorter_interval_case(
          cache, beta_minus_1);
  ret_value.exponent = minus_k;

  // When tie occurs, choose one of them according to the rule
  if (exponent >= float_info<T>::shorter_interval_tie_lower_threshold &&
      exponent <= float_info<T>::shorter_interval_tie_upper_threshold) {
    ret_value.significand = ret_value.significand % 2 == 0
                                ? ret_value.significand
                                : ret_value.significand - 1;
  } else if (ret_value.significand < xi) {
    ++ret_value.significand;
  }
  return ret_value;
}

template <typename T> decimal_fp<T> to_decimal(T x) FMT_NOEXCEPT {
  // Step 1: integer promotion & Schubfach multiplier calculation.

  using carrier_uint = typename float_info<T>::carrier_uint;
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  auto br = bit_cast<carrier_uint>(x);

  // Extract significand bits and exponent bits.
  const carrier_uint significand_mask =
      (static_cast<carrier_uint>(1) << float_info<T>::significand_bits) - 1;
  carrier_uint significand = (br & significand_mask);
  int exponent = static_cast<int>((br & exponent_mask<T>()) >>
                                  float_info<T>::significand_bits);

  if (exponent != 0) {  // Check if normal.
    exponent += float_info<T>::exponent_bias - float_info<T>::significand_bits;

    // Shorter interval case; proceed like Schubfach.
    if (significand == 0) return shorter_interval_case<T>(exponent);

    significand |=
        (static_cast<carrier_uint>(1) << float_info<T>::significand_bits);
  } else {
    // Subnormal case; the interval is always regular.
    if (significand == 0) return {0, 0};
    exponent = float_info<T>::min_exponent - float_info<T>::significand_bits;
  }

  const bool include_left_endpoint = (significand % 2 == 0);
  const bool include_right_endpoint = include_left_endpoint;

  // Compute k and beta.
  const int minus_k = floor_log10_pow2(exponent) - float_info<T>::kappa;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);
  const int beta_minus_1 = exponent + floor_log2_pow10(-minus_k);

  // Compute zi and deltai
  // 10^kappa <= deltai < 10^(kappa + 1)
  const uint32_t deltai = cache_accessor<T>::compute_delta(cache, beta_minus_1);
  const carrier_uint two_fc = significand << 1;
  const carrier_uint two_fr = two_fc | 1;
  const carrier_uint zi =
      cache_accessor<T>::compute_mul(two_fr << beta_minus_1, cache);

  // Step 2: Try larger divisor; remove trailing zeros if necessary

  // Using an upper bound on zi, we might be able to optimize the division
  // better than the compiler; we are computing zi / big_divisor here
  decimal_fp<T> ret_value;
  ret_value.significand = divide_by_10_to_kappa_plus_1(zi);
  uint32_t r = static_cast<uint32_t>(zi - float_info<T>::big_divisor *
                                              ret_value.significand);

  if (r > deltai) {
    goto small_divisor_case_label;
  } else if (r < deltai) {
    // Exclude the right endpoint if necessary
    if (r == 0 && !include_right_endpoint &&
        is_endpoint_integer<T>(two_fr, exponent, minus_k)) {
      --ret_value.significand;
      r = float_info<T>::big_divisor;
      goto small_divisor_case_label;
    }
  } else {
    // r == deltai; compare fractional parts
    // Check conditions in the order different from the paper
    // to take advantage of short-circuiting
    const carrier_uint two_fl = two_fc - 1;
    if ((!include_left_endpoint ||
         !is_endpoint_integer<T>(two_fl, exponent, minus_k)) &&
        !cache_accessor<T>::compute_mul_parity(two_fl, cache, beta_minus_1)) {
      goto small_divisor_case_label;
    }
  }
  ret_value.exponent = minus_k + float_info<T>::kappa + 1;

  // We may need to remove trailing zeros
  ret_value.exponent += remove_trailing_zeros(ret_value.significand);
  return ret_value;

  // Step 3: Find the significand with the smaller divisor

small_divisor_case_label:
  ret_value.significand *= 10;
  ret_value.exponent = minus_k + float_info<T>::kappa;

  const uint32_t mask = (1u << float_info<T>::kappa) - 1;
  auto dist = r - (deltai / 2) + (float_info<T>::small_divisor / 2);

  // Is dist divisible by 2^kappa?
  if ((dist & mask) == 0) {
    const bool approx_y_parity =
        ((dist ^ (float_info<T>::small_divisor / 2)) & 1) != 0;
    dist >>= float_info<T>::kappa;

    // Is dist divisible by 5^kappa?
    if (check_divisibility_and_divide_by_pow5<float_info<T>::kappa>(dist)) {
      ret_value.significand += dist;

      // Check z^(f) >= epsilon^(f)
      // We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
      // where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f)
      // Since there are only 2 possibilities, we only need to care about the
      // parity. Also, zi and r should have the same parity since the divisor
      // is an even number
      if (cache_accessor<T>::compute_mul_parity(two_fc, cache, beta_minus_1) !=
          approx_y_parity) {
        --ret_value.significand;
      } else {
        // If z^(f) >= epsilon^(f), we might have a tie
        // when z^(f) == epsilon^(f), or equivalently, when y is an integer
        if (is_center_integer<T>(two_fc, exponent, minus_k)) {
          ret_value.significand = ret_value.significand % 2 == 0
                                      ? ret_value.significand
                                      : ret_value.significand - 1;
        }
      }
    }
    // Is dist not divisible by 5^kappa?
    else {
      ret_value.significand += dist;
    }
  }
  // Is dist not divisible by 2^kappa?
  else {
    // Since we know dist is small, we might be able to optimize the division
    // better than the compiler; we are computing dist / small_divisor here
    ret_value.significand +=
        small_division_by_pow10<float_info<T>::kappa>(dist);
  }
  return ret_value;
}
}  // namespace dragonbox

// Formats a floating-point number using a variation of the Fixed-Precision
// Positive Floating-Point Printout ((FPP)^2) algorithm by Steele & White:
// https://fmt.dev/papers/p372-steele.pdf.
FMT_CONSTEXPR20 inline void format_dragon(fp value, bool is_predecessor_closer,
                                          int num_digits, buffer<char>& buf,
                                          int& exp10) {
  bigint numerator;    // 2 * R in (FPP)^2.
  bigint denominator;  // 2 * S in (FPP)^2.
  // lower and upper are differences between value and corresponding boundaries.
  bigint lower;             // (M^- in (FPP)^2).
  bigint upper_store;       // upper's value if different from lower.
  bigint* upper = nullptr;  // (M^+ in (FPP)^2).
  // Shift numerator and denominator by an extra bit or two (if lower boundary
  // is closer) to make lower and upper integers. This eliminates multiplication
  // by 2 during later computations.
  int shift = is_predecessor_closer ? 2 : 1;
  uint64_t significand = value.f << shift;
  if (value.e >= 0) {
    numerator.assign(significand);
    numerator <<= value.e;
    lower.assign(1);
    lower <<= value.e;
    if (shift != 1) {
      upper_store.assign(1);
      upper_store <<= value.e + 1;
      upper = &upper_store;
    }
    denominator.assign_pow10(exp10);
    denominator <<= shift;
  } else if (exp10 < 0) {
    numerator.assign_pow10(-exp10);
    lower.assign(numerator);
    if (shift != 1) {
      upper_store.assign(numerator);
      upper_store <<= 1;
      upper = &upper_store;
    }
    numerator *= significand;
    denominator.assign(1);
    denominator <<= shift - value.e;
  } else {
    numerator.assign(significand);
    denominator.assign_pow10(exp10);
    denominator <<= shift - value.e;
    lower.assign(1);
    if (shift != 1) {
      upper_store.assign(1ULL << 1);
      upper = &upper_store;
    }
  }
  // Invariant: value == (numerator / denominator) * pow(10, exp10).
  if (num_digits < 0) {
    // Generate the shortest representation.
    if (!upper) upper = &lower;
    bool even = (value.f & 1) == 0;
    num_digits = 0;
    char* data = buf.data();
    for (;;) {
      int digit = numerator.divmod_assign(denominator);
      bool low = compare(numerator, lower) - even < 0;  // numerator <[=] lower.
      // numerator + upper >[=] pow10:
      bool high = add_compare(numerator, *upper, denominator) + even > 0;
      data[num_digits++] = static_cast<char>('0' + digit);
      if (low || high) {
        if (!low) {
          ++data[num_digits - 1];
        } else if (high) {
          int result = add_compare(numerator, numerator, denominator);
          // Round half to even.
          if (result > 0 || (result == 0 && (digit % 2) != 0))
            ++data[num_digits - 1];
        }
        buf.try_resize(to_unsigned(num_digits));
        exp10 -= num_digits - 1;
        return;
      }
      numerator *= 10;
      lower *= 10;
      if (upper != &lower) *upper *= 10;
    }
  }
  // Generate the given number of digits.
  exp10 -= num_digits - 1;
  if (num_digits == 0) {
    denominator *= 10;
    auto digit = add_compare(numerator, numerator, denominator) > 0 ? '1' : '0';
    buf.push_back(digit);
    return;
  }
  buf.try_resize(to_unsigned(num_digits));
  for (int i = 0; i < num_digits - 1; ++i) {
    int digit = numerator.divmod_assign(denominator);
    buf[i] = static_cast<char>('0' + digit);
    numerator *= 10;
  }
  int digit = numerator.divmod_assign(denominator);
  auto result = add_compare(numerator, numerator, denominator);
  if (result > 0 || (result == 0 && (digit % 2) != 0)) {
    if (digit == 9) {
      const auto overflow = '0' + 10;
      buf[num_digits - 1] = overflow;
      // Propagate the carry.
      for (int i = num_digits - 1; i > 0 && buf[i] == overflow; --i) {
        buf[i] = '0';
        ++buf[i - 1];
      }
      if (buf[0] == overflow) {
        buf[0] = '1';
        ++exp10;
      }
      return;
    }
    ++digit;
  }
  buf[num_digits - 1] = static_cast<char>('0' + digit);
}

template <typename Float>
FMT_HEADER_ONLY_CONSTEXPR20 int format_float(Float value, int precision,
                                             float_specs specs,
                                             buffer<char>& buf) {
  // float is passed as double to reduce the number of instantiations.
  static_assert(!std::is_same<Float, float>::value, "");
  FMT_ASSERT(value >= 0, "value is negative");

  const bool fixed = specs.format == float_format::fixed;
  if (value <= 0) {  // <= instead of == to silence a warning.
    if (precision <= 0 || !fixed) {
      buf.push_back('0');
      return 0;
    }
    buf.try_resize(to_unsigned(precision));
    fill_n(buf.data(), precision, '0');
    return -precision;
  }

  if (specs.fallback) return snprintf_float(value, precision, specs, buf);

  if (!is_constant_evaluated() && precision < 0) {
    // Use Dragonbox for the shortest format.
    if (specs.binary32) {
      auto dec = dragonbox::to_decimal(static_cast<float>(value));
      write<char>(buffer_appender<char>(buf), dec.significand);
      return dec.exponent;
    }
    auto dec = dragonbox::to_decimal(static_cast<double>(value));
    write<char>(buffer_appender<char>(buf), dec.significand);
    return dec.exponent;
  }

  int exp = 0;
  bool use_dragon = true;
  if (is_fast_float<Float>()) {
    // Use Grisu + Dragon4 for the given precision:
    // https://www.cs.tufts.edu/~nr/cs257/archive/florian-loitsch/printf.pdf.
    const int min_exp = -60;  // alpha in Grisu.
    int cached_exp10 = 0;     // K in Grisu.
    fp normalized = normalize(fp(value));
    const auto cached_pow = get_cached_power(
        min_exp - (normalized.e + fp::num_significand_bits), cached_exp10);
    normalized = normalized * cached_pow;
    gen_digits_handler handler{buf.data(), 0, precision, -cached_exp10, fixed};
    if (grisu_gen_digits(normalized, 1, exp, handler) != digits::error &&
        !is_constant_evaluated()) {
      exp += handler.exp10;
      buf.try_resize(to_unsigned(handler.size));
      use_dragon = false;
    } else {
      exp += handler.size - cached_exp10 - 1;
      precision = handler.precision;
    }
  }
  if (use_dragon) {
    auto f = fp();
    bool is_predecessor_closer =
        specs.binary32 ? f.assign(static_cast<float>(value)) : f.assign(value);
    // Limit precision to the maximum possible number of significant digits in
    // an IEEE754 double because we don't need to generate zeros.
    const int max_double_digits = 767;
    if (precision > max_double_digits) precision = max_double_digits;
    format_dragon(f, is_predecessor_closer, precision, buf, exp);
  }
  if (!fixed && !specs.showpoint) {
    // Remove trailing zeros.
    auto num_digits = buf.size();
    while (num_digits > 0 && buf[num_digits - 1] == '0') {
      --num_digits;
      ++exp;
    }
    buf.try_resize(num_digits);
  }
  return exp;
}

template <typename T>
int snprintf_float(T value, int precision, float_specs specs,
                   buffer<char>& buf) {
  // Buffer capacity must be non-zero, otherwise MSVC's vsnprintf_s will fail.
  FMT_ASSERT(buf.capacity() > buf.size(), "empty buffer");
  static_assert(!std::is_same<T, float>::value, "");

  // Subtract 1 to account for the difference in precision since we use %e for
  // both general and exponent format.
  if (specs.format == float_format::general ||
      specs.format == float_format::exp)
    precision = (precision >= 0 ? precision : 6) - 1;

  // Build the format string.
  enum { max_format_size = 7 };  // The longest format is "%#.*Le".
  char format[max_format_size];
  char* format_ptr = format;
  *format_ptr++ = '%';
  if (specs.showpoint && specs.format == float_format::hex) *format_ptr++ = '#';
  if (precision >= 0) {
    *format_ptr++ = '.';
    *format_ptr++ = '*';
  }
  if (std::is_same<T, long double>()) *format_ptr++ = 'L';
  *format_ptr++ = specs.format != float_format::hex
                      ? (specs.format == float_format::fixed ? 'f' : 'e')
                      : (specs.upper ? 'A' : 'a');
  *format_ptr = '\0';

  // Format using snprintf.
  auto offset = buf.size();
  for (;;) {
    auto begin = buf.data() + offset;
    auto capacity = buf.capacity() - offset;
#ifdef FMT_FUZZ
    if (precision > 100000)
      throw std::runtime_error(
          "fuzz mode - avoid large allocation inside snprintf");
#endif
    // Suppress the warning about a nonliteral format string.
    // Cannot use auto because of a bug in MinGW (#1532).
    int (*snprintf_ptr)(char*, size_t, const char*, ...) = FMT_SNPRINTF;
    int result = precision >= 0
                     ? snprintf_ptr(begin, capacity, format, precision, value)
                     : snprintf_ptr(begin, capacity, format, value);
    if (result < 0) {
      // The buffer will grow exponentially.
      buf.try_reserve(buf.capacity() + 1);
      continue;
    }
    auto size = to_unsigned(result);
    // Size equal to capacity means that the last character was truncated.
    if (size >= capacity) {
      buf.try_reserve(size + offset + 1);  // Add 1 for the terminating '\0'.
      continue;
    }
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
    if (specs.format == float_format::fixed) {
      if (precision == 0) {
        buf.try_resize(size);
        return 0;
      }
      // Find and remove the decimal point.
      auto end = begin + size, p = end;
      do {
        --p;
      } while (is_digit(*p));
      int fraction_size = static_cast<int>(end - p - 1);
      std::memmove(p, p + 1, to_unsigned(fraction_size));
      buf.try_resize(size - 1);
      return -fraction_size;
    }
    if (specs.format == float_format::hex) {
      buf.try_resize(size + offset);
      return 0;
    }
    // Find and parse the exponent.
    auto end = begin + size, exp_pos = end;
    do {
      --exp_pos;
    } while (*exp_pos != 'e');
    char sign = exp_pos[1];
    FMT_ASSERT(sign == '+' || sign == '-', "");
    int exp = 0;
    auto p = exp_pos + 2;  // Skip 'e' and sign.
    do {
      FMT_ASSERT(is_digit(*p), "");
      exp = exp * 10 + (*p++ - '0');
    } while (p != end);
    if (sign == '-') exp = -exp;
    int fraction_size = 0;
    if (exp_pos != begin + 1) {
      // Remove trailing zeros.
      auto fraction_end = exp_pos - 1;
      while (*fraction_end == '0') --fraction_end;
      // Move the fractional part left to get rid of the decimal point.
      fraction_size = static_cast<int>(fraction_end - begin - 1);
      std::memmove(begin + 1, begin + 2, to_unsigned(fraction_size));
    }
    buf.try_resize(to_unsigned(fraction_size) + offset + 1);
    return exp - fraction_size;
  }
}
}  // namespace detail

FMT_FUNC detail::utf8_to_utf16::utf8_to_utf16(string_view s) {
  for_each_codepoint(s, [this](uint32_t cp, string_view) {
    if (cp == invalid_code_point) FMT_THROW(std::runtime_error("invalid utf8"));
    if (cp <= 0xFFFF) {
      buffer_.push_back(static_cast<wchar_t>(cp));
    } else {
      cp -= 0x10000;
      buffer_.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
      buffer_.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
    }
    return true;
  });
  buffer_.push_back(0);
}

FMT_FUNC void format_system_error(detail::buffer<char>& out, int error_code,
                                  const char* message) FMT_NOEXCEPT {
  FMT_TRY {
    auto ec = std::error_code(error_code, std::generic_category());
    write(std::back_inserter(out), std::system_error(ec, message).what());
    return;
  }
  FMT_CATCH(...) {}
  format_error_code(out, error_code, message);
}

FMT_FUNC void report_system_error(int error_code,
                                  const char* message) FMT_NOEXCEPT {
  report_error(format_system_error, error_code, message);
}

// DEPRECATED!
// This function is defined here and not inline for ABI compatiblity.
FMT_FUNC void detail::error_handler::on_error(const char* message) {
  throw_format_error(message);
}

FMT_FUNC std::string vformat(string_view fmt, format_args args) {
  // Don't optimize the "{}" case to keep the binary size small and because it
  // can be better optimized in fmt::format anyway.
  auto buffer = memory_buffer();
  detail::vformat_to(buffer, fmt, args);
  return to_string(buffer);
}

#ifdef _WIN32
namespace detail {
using dword = conditional_t<sizeof(long) == 4, unsigned long, unsigned>;
extern "C" __declspec(dllimport) int __stdcall WriteConsoleW(  //
    void*, const void*, dword, dword*, void*);
}  // namespace detail
#endif

namespace detail {
FMT_FUNC void print(std::FILE* f, string_view text) {
#ifdef _WIN32
  auto fd = _fileno(f);
  if (_isatty(fd)) {
    detail::utf8_to_utf16 u16(string_view(text.data(), text.size()));
    auto written = detail::dword();
    if (detail::WriteConsoleW(reinterpret_cast<void*>(_get_osfhandle(fd)),
                              u16.c_str(), static_cast<uint32_t>(u16.size()),
                              &written, nullptr)) {
      return;
    }
    // Fallback to fwrite on failure. It can happen if the output has been
    // redirected to NUL.
  }
#endif
  detail::fwrite_fully(text.data(), 1, text.size(), f);
}
}  // namespace detail

FMT_FUNC void vprint(std::FILE* f, string_view format_str, format_args args) {
  memory_buffer buffer;
  detail::vformat_to(buffer, format_str, args);
  detail::print(f, {buffer.data(), buffer.size()});
}

#ifdef _WIN32
// Print assuming legacy (non-Unicode) encoding.
FMT_FUNC void detail::vprint_mojibake(std::FILE* f, string_view format_str,
                                      format_args args) {
  memory_buffer buffer;
  detail::vformat_to(buffer, format_str,
                     basic_format_args<buffer_context<char>>(args));
  fwrite_fully(buffer.data(), 1, buffer.size(), f);
}
#endif

FMT_FUNC void vprint(string_view format_str, format_args args) {
  vprint(stdout, format_str, args);
}

FMT_END_NAMESPACE

#endif  // FMT_FORMAT_INL_H_
