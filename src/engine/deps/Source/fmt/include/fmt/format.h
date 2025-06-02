/*
 Formatting library for C++

 Copyright (c) 2012 - present, Victor Zverovich

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 --- Optional exception to the license ---

 As an exception, if, as a result of your compiling your source code, portions
 of this Software are embedded into a machine-executable object form of such
 source code, you may redistribute such embedded portions in such object form
 without including the above copyright and permission notices.
 */

#ifndef FMT_FORMAT_H_
#define FMT_FORMAT_H_

#include <cmath>         // std::signbit
#include <cstdint>       // uint32_t
#include <limits>        // std::numeric_limits
#include <memory>        // std::uninitialized_copy
#include <stdexcept>     // std::runtime_error
#include <system_error>  // std::system_error
#include <utility>       // std::swap

#ifdef __cpp_lib_bit_cast
#  include <bit>  // std::bitcast
#endif

#include "core.h"

#if FMT_GCC_VERSION
#  define FMT_GCC_VISIBILITY_HIDDEN __attribute__((visibility("hidden")))
#else
#  define FMT_GCC_VISIBILITY_HIDDEN
#endif

#ifdef __NVCC__
#  define FMT_CUDA_VERSION (__CUDACC_VER_MAJOR__ * 100 + __CUDACC_VER_MINOR__)
#else
#  define FMT_CUDA_VERSION 0
#endif

#ifdef __has_builtin
#  define FMT_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define FMT_HAS_BUILTIN(x) 0
#endif

#if FMT_GCC_VERSION || FMT_CLANG_VERSION
#  define FMT_NOINLINE __attribute__((noinline))
#else
#  define FMT_NOINLINE
#endif

#if FMT_MSC_VER
#  define FMT_MSC_DEFAULT = default
#else
#  define FMT_MSC_DEFAULT
#endif

#ifndef FMT_THROW
#  if FMT_EXCEPTIONS
#    if FMT_MSC_VER || FMT_NVCC
FMT_BEGIN_NAMESPACE
namespace detail {
template <typename Exception> inline void do_throw(const Exception& x) {
  // Silence unreachable code warnings in MSVC and NVCC because these
  // are nearly impossible to fix in a generic code.
  volatile bool b = true;
  if (b) throw x;
}
}  // namespace detail
FMT_END_NAMESPACE
#      define FMT_THROW(x) detail::do_throw(x)
#    else
#      define FMT_THROW(x) throw x
#    endif
#  else
#    define FMT_THROW(x)               \
      do {                             \
        FMT_ASSERT(false, (x).what()); \
      } while (false)
#  endif
#endif

#if FMT_EXCEPTIONS
#  define FMT_TRY try
#  define FMT_CATCH(x) catch (x)
#else
#  define FMT_TRY if (true)
#  define FMT_CATCH(x) if (false)
#endif

#ifndef FMT_MAYBE_UNUSED
#  if FMT_HAS_CPP17_ATTRIBUTE(maybe_unused)
#    define FMT_MAYBE_UNUSED [[maybe_unused]]
#  else
#    define FMT_MAYBE_UNUSED
#  endif
#endif

// Workaround broken [[deprecated]] in the Intel, PGI and NVCC compilers.
#if FMT_ICC_VERSION || defined(__PGI) || FMT_NVCC
#  define FMT_DEPRECATED_ALIAS
#else
#  define FMT_DEPRECATED_ALIAS FMT_DEPRECATED
#endif

#ifndef FMT_USE_USER_DEFINED_LITERALS
// EDG based compilers (Intel, NVIDIA, Elbrus, etc), GCC and MSVC support UDLs.
#  if (FMT_HAS_FEATURE(cxx_user_literals) || FMT_GCC_VERSION >= 407 || \
       FMT_MSC_VER >= 1900) &&                                         \
      (!defined(__EDG_VERSION__) || __EDG_VERSION__ >= /* UDL feature */ 480)
#    define FMT_USE_USER_DEFINED_LITERALS 1
#  else
#    define FMT_USE_USER_DEFINED_LITERALS 0
#  endif
#endif

// Defining FMT_REDUCE_INT_INSTANTIATIONS to 1, will reduce the number of
// integer formatter template instantiations to just one by only using the
// largest integer type. This results in a reduction in binary size but will
// cause a decrease in integer formatting performance.
#if !defined(FMT_REDUCE_INT_INSTANTIATIONS)
#  define FMT_REDUCE_INT_INSTANTIATIONS 0
#endif

// __builtin_clz is broken in clang with Microsoft CodeGen:
// https://github.com/fmtlib/fmt/issues/519.
#if !FMT_MSC_VER
#  if FMT_HAS_BUILTIN(__builtin_clz) || FMT_GCC_VERSION || FMT_ICC_VERSION
#    define FMT_BUILTIN_CLZ(n) __builtin_clz(n)
#  endif
#  if FMT_HAS_BUILTIN(__builtin_clzll) || FMT_GCC_VERSION || FMT_ICC_VERSION
#    define FMT_BUILTIN_CLZLL(n) __builtin_clzll(n)
#  endif
#endif

// __builtin_ctz is broken in Intel Compiler Classic on Windows:
// https://github.com/fmtlib/fmt/issues/2510.
#ifndef __ICL
#  if FMT_HAS_BUILTIN(__builtin_ctz) || FMT_GCC_VERSION || FMT_ICC_VERSION
#    define FMT_BUILTIN_CTZ(n) __builtin_ctz(n)
#  endif
#  if FMT_HAS_BUILTIN(__builtin_ctzll) || FMT_GCC_VERSION || FMT_ICC_VERSION
#    define FMT_BUILTIN_CTZLL(n) __builtin_ctzll(n)
#  endif
#endif

#if FMT_MSC_VER
#  include <intrin.h>  // _BitScanReverse[64], _BitScanForward[64], _umul128
#endif

// Some compilers masquerade as both MSVC and GCC-likes or otherwise support
// __builtin_clz and __builtin_clzll, so only define FMT_BUILTIN_CLZ using the
// MSVC intrinsics if the clz and clzll builtins are not available.
#if FMT_MSC_VER && !defined(FMT_BUILTIN_CLZLL) && !defined(FMT_BUILTIN_CTZLL)
FMT_BEGIN_NAMESPACE
namespace detail {
// Avoid Clang with Microsoft CodeGen's -Wunknown-pragmas warning.
#  if !defined(__clang__)
#    pragma intrinsic(_BitScanForward)
#    pragma intrinsic(_BitScanReverse)
#    if defined(_WIN64)
#      pragma intrinsic(_BitScanForward64)
#      pragma intrinsic(_BitScanReverse64)
#    endif
#  endif

inline auto clz(uint32_t x) -> int {
  unsigned long r = 0;
  _BitScanReverse(&r, x);
  FMT_ASSERT(x != 0, "");
  // Static analysis complains about using uninitialized data
  // "r", but the only way that can happen is if "x" is 0,
  // which the callers guarantee to not happen.
  FMT_MSC_WARNING(suppress : 6102)
  return 31 ^ static_cast<int>(r);
}
#  define FMT_BUILTIN_CLZ(n) detail::clz(n)

inline auto clzll(uint64_t x) -> int {
  unsigned long r = 0;
#  ifdef _WIN64
  _BitScanReverse64(&r, x);
#  else
  // Scan the high 32 bits.
  if (_BitScanReverse(&r, static_cast<uint32_t>(x >> 32))) return 63 ^ (r + 32);
  // Scan the low 32 bits.
  _BitScanReverse(&r, static_cast<uint32_t>(x));
#  endif
  FMT_ASSERT(x != 0, "");
  FMT_MSC_WARNING(suppress : 6102)  // Suppress a bogus static analysis warning.
  return 63 ^ static_cast<int>(r);
}
#  define FMT_BUILTIN_CLZLL(n) detail::clzll(n)

inline auto ctz(uint32_t x) -> int {
  unsigned long r = 0;
  _BitScanForward(&r, x);
  FMT_ASSERT(x != 0, "");
  FMT_MSC_WARNING(suppress : 6102)  // Suppress a bogus static analysis warning.
  return static_cast<int>(r);
}
#  define FMT_BUILTIN_CTZ(n) detail::ctz(n)

inline auto ctzll(uint64_t x) -> int {
  unsigned long r = 0;
  FMT_ASSERT(x != 0, "");
  FMT_MSC_WARNING(suppress : 6102)  // Suppress a bogus static analysis warning.
#  ifdef _WIN64
  _BitScanForward64(&r, x);
#  else
  // Scan the low 32 bits.
  if (_BitScanForward(&r, static_cast<uint32_t>(x))) return static_cast<int>(r);
  // Scan the high 32 bits.
  _BitScanForward(&r, static_cast<uint32_t>(x >> 32));
  r += 32;
#  endif
  return static_cast<int>(r);
}
#  define FMT_BUILTIN_CTZLL(n) detail::ctzll(n)
}  // namespace detail
FMT_END_NAMESPACE
#endif

#ifdef FMT_HEADER_ONLY
#  define FMT_HEADER_ONLY_CONSTEXPR20 FMT_CONSTEXPR20
#else
#  define FMT_HEADER_ONLY_CONSTEXPR20
#endif

FMT_BEGIN_NAMESPACE
namespace detail {

template <typename Streambuf> class formatbuf : public Streambuf {
 private:
  using char_type = typename Streambuf::char_type;
  using streamsize = decltype(std::declval<Streambuf>().sputn(nullptr, 0));
  using int_type = typename Streambuf::int_type;
  using traits_type = typename Streambuf::traits_type;

  buffer<char_type>& buffer_;

 public:
  explicit formatbuf(buffer<char_type>& buf) : buffer_(buf) {}

 protected:
  // The put area is always empty. This makes the implementation simpler and has
  // the advantage that the streambuf and the buffer are always in sync and
  // sputc never writes into uninitialized memory. A disadvantage is that each
  // call to sputc always results in a (virtual) call to overflow. There is no
  // disadvantage here for sputn since this always results in a call to xsputn.

  auto overflow(int_type ch) -> int_type override {
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
      buffer_.push_back(static_cast<char_type>(ch));
    return ch;
  }

  auto xsputn(const char_type* s, streamsize count) -> streamsize override {
    buffer_.append(s, s + count);
    return count;
  }
};

// Implementation of std::bit_cast for pre-C++20.
template <typename To, typename From>
FMT_CONSTEXPR20 auto bit_cast(const From& from) -> To {
  static_assert(sizeof(To) == sizeof(From), "size mismatch");
#ifdef __cpp_lib_bit_cast
  if (is_constant_evaluated()) return std::bit_cast<To>(from);
#endif
  auto to = To();
  std::memcpy(&to, &from, sizeof(to));
  return to;
}

inline auto is_big_endian() -> bool {
#ifdef _WIN32
  return false;
#elif defined(__BIG_ENDIAN__)
  return true;
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
  return __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__;
#else
  struct bytes {
    char data[sizeof(int)];
  };
  return bit_cast<bytes>(1).data[0] == 0;
#endif
}

// A fallback implementation of uintptr_t for systems that lack it.
struct fallback_uintptr {
  unsigned char value[sizeof(void*)];

  fallback_uintptr() = default;
  explicit fallback_uintptr(const void* p) {
    *this = bit_cast<fallback_uintptr>(p);
    if (const_check(is_big_endian())) {
      for (size_t i = 0, j = sizeof(void*) - 1; i < j; ++i, --j)
        std::swap(value[i], value[j]);
    }
  }
};
#ifdef UINTPTR_MAX
using uintptr_t = ::uintptr_t;
inline auto to_uintptr(const void* p) -> uintptr_t {
  return bit_cast<uintptr_t>(p);
}
#else
using uintptr_t = fallback_uintptr;
inline auto to_uintptr(const void* p) -> fallback_uintptr {
  return fallback_uintptr(p);
}
#endif

// Returns the largest possible value for type T. Same as
// std::numeric_limits<T>::max() but shorter and not affected by the max macro.
template <typename T> constexpr auto max_value() -> T {
  return (std::numeric_limits<T>::max)();
}
template <typename T> constexpr auto num_bits() -> int {
  return std::numeric_limits<T>::digits;
}
// std::numeric_limits<T>::digits may return 0 for 128-bit ints.
template <> constexpr auto num_bits<int128_t>() -> int { return 128; }
template <> constexpr auto num_bits<uint128_t>() -> int { return 128; }
template <> constexpr auto num_bits<fallback_uintptr>() -> int {
  return static_cast<int>(sizeof(void*) *
                          std::numeric_limits<unsigned char>::digits);
}

FMT_INLINE void assume(bool condition) {
  (void)condition;
#if FMT_HAS_BUILTIN(__builtin_assume)
  __builtin_assume(condition);
#endif
}

// An approximation of iterator_t for pre-C++20 systems.
template <typename T>
using iterator_t = decltype(std::begin(std::declval<T&>()));
template <typename T> using sentinel_t = decltype(std::end(std::declval<T&>()));

// A workaround for std::string not having mutable data() until C++17.
template <typename Char>
inline auto get_data(std::basic_string<Char>& s) -> Char* {
  return &s[0];
}
template <typename Container>
inline auto get_data(Container& c) -> typename Container::value_type* {
  return c.data();
}

#if defined(_SECURE_SCL) && _SECURE_SCL
// Make a checked iterator to avoid MSVC warnings.
template <typename T> using checked_ptr = stdext::checked_array_iterator<T*>;
template <typename T>
constexpr auto make_checked(T* p, size_t size) -> checked_ptr<T> {
  return {p, size};
}
#else
template <typename T> using checked_ptr = T*;
template <typename T> constexpr auto make_checked(T* p, size_t) -> T* {
  return p;
}
#endif

// Attempts to reserve space for n extra characters in the output range.
// Returns a pointer to the reserved range or a reference to it.
template <typename Container, FMT_ENABLE_IF(is_contiguous<Container>::value)>
#if FMT_CLANG_VERSION >= 307 && !FMT_ICC_VERSION
__attribute__((no_sanitize("undefined")))
#endif
inline auto
reserve(std::back_insert_iterator<Container> it, size_t n)
    -> checked_ptr<typename Container::value_type> {
  Container& c = get_container(it);
  size_t size = c.size();
  c.resize(size + n);
  return make_checked(get_data(c) + size, n);
}

template <typename T>
inline auto reserve(buffer_appender<T> it, size_t n) -> buffer_appender<T> {
  buffer<T>& buf = get_container(it);
  buf.try_reserve(buf.size() + n);
  return it;
}

template <typename Iterator>
constexpr auto reserve(Iterator& it, size_t) -> Iterator& {
  return it;
}

template <typename OutputIt>
using reserve_iterator =
    remove_reference_t<decltype(reserve(std::declval<OutputIt&>(), 0))>;

template <typename T, typename OutputIt>
constexpr auto to_pointer(OutputIt, size_t) -> T* {
  return nullptr;
}
template <typename T> auto to_pointer(buffer_appender<T> it, size_t n) -> T* {
  buffer<T>& buf = get_container(it);
  auto size = buf.size();
  if (buf.capacity() < size + n) return nullptr;
  buf.try_resize(size + n);
  return buf.data() + size;
}

template <typename Container, FMT_ENABLE_IF(is_contiguous<Container>::value)>
inline auto base_iterator(std::back_insert_iterator<Container>& it,
                          checked_ptr<typename Container::value_type>)
    -> std::back_insert_iterator<Container> {
  return it;
}

template <typename Iterator>
constexpr auto base_iterator(Iterator, Iterator it) -> Iterator {
  return it;
}

// <algorithm> is spectacularly slow to compile in C++20 so use a simple fill_n
// instead (#1998).
template <typename OutputIt, typename Size, typename T>
FMT_CONSTEXPR auto fill_n(OutputIt out, Size count, const T& value)
    -> OutputIt {
  for (Size i = 0; i < count; ++i) *out++ = value;
  return out;
}
template <typename T, typename Size>
FMT_CONSTEXPR20 auto fill_n(T* out, Size count, char value) -> T* {
  if (is_constant_evaluated()) {
    return fill_n<T*, Size, T>(out, count, value);
  }
  std::memset(out, value, to_unsigned(count));
  return out + count;
}

#ifdef __cpp_char8_t
using char8_type = char8_t;
#else
enum char8_type : unsigned char {};
#endif

template <typename OutChar, typename InputIt, typename OutputIt>
FMT_CONSTEXPR FMT_NOINLINE auto copy_str_noinline(InputIt begin, InputIt end,
                                                  OutputIt out) -> OutputIt {
  return copy_str<OutChar>(begin, end, out);
}

// A public domain branchless UTF-8 decoder by Christopher Wellons:
// https://github.com/skeeto/branchless-utf8
/* Decode the next character, c, from s, reporting errors in e.
 *
 * Since this is a branchless decoder, four bytes will be read from the
 * buffer regardless of the actual length of the next character. This
 * means the buffer _must_ have at least three bytes of zero padding
 * following the end of the data stream.
 *
 * Errors are reported in e, which will be non-zero if the parsed
 * character was somehow invalid: invalid byte sequence, non-canonical
 * encoding, or a surrogate half.
 *
 * The function returns a pointer to the next character. When an error
 * occurs, this pointer will be a guess that depends on the particular
 * error, but it will always advance at least one byte.
 */
FMT_CONSTEXPR inline auto utf8_decode(const char* s, uint32_t* c, int* e)
    -> const char* {
  constexpr const int masks[] = {0x00, 0x7f, 0x1f, 0x0f, 0x07};
  constexpr const uint32_t mins[] = {4194304, 0, 128, 2048, 65536};
  constexpr const int shiftc[] = {0, 18, 12, 6, 0};
  constexpr const int shifte[] = {0, 6, 4, 2, 0};

  int len = code_point_length(s);
  const char* next = s + len;

  // Assume a four-byte character and load four bytes. Unused bits are
  // shifted out.
  *c = uint32_t(s[0] & masks[len]) << 18;
  *c |= uint32_t(s[1] & 0x3f) << 12;
  *c |= uint32_t(s[2] & 0x3f) << 6;
  *c |= uint32_t(s[3] & 0x3f) << 0;
  *c >>= shiftc[len];

  // Accumulate the various error conditions.
  using uchar = unsigned char;
  *e = (*c < mins[len]) << 6;       // non-canonical encoding
  *e |= ((*c >> 11) == 0x1b) << 7;  // surrogate half?
  *e |= (*c > 0x10FFFF) << 8;       // out of range?
  *e |= (uchar(s[1]) & 0xc0) >> 2;
  *e |= (uchar(s[2]) & 0xc0) >> 4;
  *e |= uchar(s[3]) >> 6;
  *e ^= 0x2a;  // top two bits of each tail byte correct?
  *e >>= shifte[len];

  return next;
}

constexpr uint32_t invalid_code_point = ~uint32_t();

// Invokes f(cp, sv) for every code point cp in s with sv being the string view
// corresponding to the code point. cp is invalid_code_point on error.
template <typename F>
FMT_CONSTEXPR void for_each_codepoint(string_view s, F f) {
  auto decode = [f](const char* buf_ptr, const char* ptr) {
    auto cp = uint32_t();
    auto error = 0;
    auto end = utf8_decode(buf_ptr, &cp, &error);
    bool result = f(error ? invalid_code_point : cp,
                    string_view(ptr, to_unsigned(end - buf_ptr)));
    return result ? end : nullptr;
  };
  auto p = s.data();
  const size_t block_size = 4;  // utf8_decode always reads blocks of 4 chars.
  if (s.size() >= block_size) {
    for (auto end = p + s.size() - block_size + 1; p < end;) {
      p = decode(p, p);
      if (!p) return;
    }
  }
  if (auto num_chars_left = s.data() + s.size() - p) {
    char buf[2 * block_size - 1] = {};
    copy_str<char>(p, p + num_chars_left, buf);
    const char* buf_ptr = buf;
    do {
      auto end = decode(buf_ptr, p);
      if (!end) return;
      p += end - buf_ptr;
      buf_ptr = end;
    } while (buf_ptr - buf < num_chars_left);
  }
}

template <typename Char>
inline auto compute_width(basic_string_view<Char> s) -> size_t {
  return s.size();
}

// Computes approximate display width of a UTF-8 string.
FMT_CONSTEXPR inline size_t compute_width(string_view s) {
  size_t num_code_points = 0;
  // It is not a lambda for compatibility with C++14.
  struct count_code_points {
    size_t* count;
    FMT_CONSTEXPR auto operator()(uint32_t cp, string_view) const -> bool {
      *count += detail::to_unsigned(
          1 +
          (cp >= 0x1100 &&
           (cp <= 0x115f ||  // Hangul Jamo init. consonants
            cp == 0x2329 ||  // LEFT-POINTING ANGLE BRACKET
            cp == 0x232a ||  // RIGHT-POINTING ANGLE BRACKET
            // CJK ... Yi except IDEOGRAPHIC HALF FILL SPACE:
            (cp >= 0x2e80 && cp <= 0xa4cf && cp != 0x303f) ||
            (cp >= 0xac00 && cp <= 0xd7a3) ||    // Hangul Syllables
            (cp >= 0xf900 && cp <= 0xfaff) ||    // CJK Compatibility Ideographs
            (cp >= 0xfe10 && cp <= 0xfe19) ||    // Vertical Forms
            (cp >= 0xfe30 && cp <= 0xfe6f) ||    // CJK Compatibility Forms
            (cp >= 0xff00 && cp <= 0xff60) ||    // Fullwidth Forms
            (cp >= 0xffe0 && cp <= 0xffe6) ||    // Fullwidth Forms
            (cp >= 0x20000 && cp <= 0x2fffd) ||  // CJK
            (cp >= 0x30000 && cp <= 0x3fffd) ||
            // Miscellaneous Symbols and Pictographs + Emoticons:
            (cp >= 0x1f300 && cp <= 0x1f64f) ||
            // Supplemental Symbols and Pictographs:
            (cp >= 0x1f900 && cp <= 0x1f9ff))));
      return true;
    }
  };
  for_each_codepoint(s, count_code_points{&num_code_points});
  return num_code_points;
}

inline auto compute_width(basic_string_view<char8_type> s) -> size_t {
  return compute_width(basic_string_view<char>(
      reinterpret_cast<const char*>(s.data()), s.size()));
}

template <typename Char>
inline auto code_point_index(basic_string_view<Char> s, size_t n) -> size_t {
  size_t size = s.size();
  return n < size ? n : size;
}

// Calculates the index of the nth code point in a UTF-8 string.
inline auto code_point_index(basic_string_view<char8_type> s, size_t n)
    -> size_t {
  const char8_type* data = s.data();
  size_t num_code_points = 0;
  for (size_t i = 0, size = s.size(); i != size; ++i) {
    if ((data[i] & 0xc0) != 0x80 && ++num_code_points > n) return i;
  }
  return s.size();
}

template <typename T, bool = std::is_floating_point<T>::value>
struct is_fast_float : bool_constant<std::numeric_limits<T>::is_iec559 &&
                                     sizeof(T) <= sizeof(double)> {};
template <typename T> struct is_fast_float<T, false> : std::false_type {};

#ifndef FMT_USE_FULL_CACHE_DRAGONBOX
#  define FMT_USE_FULL_CACHE_DRAGONBOX 0
#endif

template <typename T>
template <typename U>
void buffer<T>::append(const U* begin, const U* end) {
  while (begin != end) {
    auto count = to_unsigned(end - begin);
    try_reserve(size_ + count);
    auto free_cap = capacity_ - size_;
    if (free_cap < count) count = free_cap;
    std::uninitialized_copy_n(begin, count, make_checked(ptr_ + size_, count));
    size_ += count;
    begin += count;
  }
}

template <typename T, typename Enable = void>
struct is_locale : std::false_type {};
template <typename T>
struct is_locale<T, void_t<decltype(T::classic())>> : std::true_type {};
}  // namespace detail

FMT_MODULE_EXPORT_BEGIN

// The number of characters to store in the basic_memory_buffer object itself
// to avoid dynamic memory allocation.
enum { inline_buffer_size = 500 };

/**
  \rst
  A dynamically growing memory buffer for trivially copyable/constructible types
  with the first ``SIZE`` elements stored in the object itself.

  You can use the ``memory_buffer`` type alias for ``char`` instead.

  **Example**::

     auto out = fmt::memory_buffer();
     format_to(std::back_inserter(out), "The answer is {}.", 42);

  This will append the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42.

  The output can be converted to an ``std::string`` with ``to_string(out)``.
  \endrst
 */
template <typename T, size_t SIZE = inline_buffer_size,
          typename Allocator = std::allocator<T>>
class basic_memory_buffer final : public detail::buffer<T> {
 private:
  T store_[SIZE];

  // Don't inherit from Allocator avoid generating type_info for it.
  Allocator alloc_;

  // Deallocate memory allocated by the buffer.
  FMT_CONSTEXPR20 void deallocate() {
    T* data = this->data();
    if (data != store_) alloc_.deallocate(data, this->capacity());
  }

 protected:
  FMT_CONSTEXPR20 void grow(size_t size) override;

 public:
  using value_type = T;
  using const_reference = const T&;

  FMT_CONSTEXPR20 explicit basic_memory_buffer(
      const Allocator& alloc = Allocator())
      : alloc_(alloc) {
    this->set(store_, SIZE);
    if (detail::is_constant_evaluated()) {
      detail::fill_n(store_, SIZE, T{});
    }
  }
  FMT_CONSTEXPR20 ~basic_memory_buffer() { deallocate(); }

 private:
  // Move data from other to this buffer.
  FMT_CONSTEXPR20 void move(basic_memory_buffer& other) {
    alloc_ = std::move(other.alloc_);
    T* data = other.data();
    size_t size = other.size(), capacity = other.capacity();
    if (data == other.store_) {
      this->set(store_, capacity);
      if (detail::is_constant_evaluated()) {
        detail::copy_str<T>(other.store_, other.store_ + size,
                            detail::make_checked(store_, capacity));
      } else {
        std::uninitialized_copy(other.store_, other.store_ + size,
                                detail::make_checked(store_, capacity));
      }
    } else {
      this->set(data, capacity);
      // Set pointer to the inline array so that delete is not called
      // when deallocating.
      other.set(other.store_, 0);
    }
    this->resize(size);
  }

 public:
  /**
    \rst
    Constructs a :class:`fmt::basic_memory_buffer` object moving the content
    of the other object to it.
    \endrst
   */
  FMT_CONSTEXPR20 basic_memory_buffer(basic_memory_buffer&& other)
      FMT_NOEXCEPT {
    move(other);
  }

  /**
    \rst
    Moves the content of the other ``basic_memory_buffer`` object to this one.
    \endrst
   */
  auto operator=(basic_memory_buffer&& other) FMT_NOEXCEPT
      -> basic_memory_buffer& {
    FMT_ASSERT(this != &other, "");
    deallocate();
    move(other);
    return *this;
  }

  // Returns a copy of the allocator associated with this buffer.
  auto get_allocator() const -> Allocator { return alloc_; }

  /**
    Resizes the buffer to contain *count* elements. If T is a POD type new
    elements may not be initialized.
   */
  FMT_CONSTEXPR20 void resize(size_t count) { this->try_resize(count); }

  /** Increases the buffer capacity to *new_capacity*. */
  void reserve(size_t new_capacity) { this->try_reserve(new_capacity); }

  // Directly append data into the buffer
  using detail::buffer<T>::append;
  template <typename ContiguousRange>
  void append(const ContiguousRange& range) {
    append(range.data(), range.data() + range.size());
  }
};

template <typename T, size_t SIZE, typename Allocator>
FMT_CONSTEXPR20 void basic_memory_buffer<T, SIZE, Allocator>::grow(
    size_t size) {
#ifdef FMT_FUZZ
  if (size > 5000) throw std::runtime_error("fuzz mode - won't grow that much");
#endif
  const size_t max_size = std::allocator_traits<Allocator>::max_size(alloc_);
  size_t old_capacity = this->capacity();
  size_t new_capacity = old_capacity + old_capacity / 2;
  if (size > new_capacity)
    new_capacity = size;
  else if (new_capacity > max_size)
    new_capacity = size > max_size ? size : max_size;
  T* old_data = this->data();
  T* new_data =
      std::allocator_traits<Allocator>::allocate(alloc_, new_capacity);
  // The following code doesn't throw, so the raw pointer above doesn't leak.
  std::uninitialized_copy(old_data, old_data + this->size(),
                          detail::make_checked(new_data, new_capacity));
  this->set(new_data, new_capacity);
  // deallocate must not throw according to the standard, but even if it does,
  // the buffer already uses the new storage and will deallocate it in
  // destructor.
  if (old_data != store_) alloc_.deallocate(old_data, old_capacity);
}

using memory_buffer = basic_memory_buffer<char>;

template <typename T, size_t SIZE, typename Allocator>
struct is_contiguous<basic_memory_buffer<T, SIZE, Allocator>> : std::true_type {
};

namespace detail {
FMT_API void print(std::FILE*, string_view);
}

/** A formatting error such as invalid format string. */
FMT_CLASS_API
class FMT_API format_error : public std::runtime_error {
 public:
  explicit format_error(const char* message) : std::runtime_error(message) {}
  explicit format_error(const std::string& message)
      : std::runtime_error(message) {}
  format_error(const format_error&) = default;
  format_error& operator=(const format_error&) = default;
  format_error(format_error&&) = default;
  format_error& operator=(format_error&&) = default;
  ~format_error() FMT_NOEXCEPT override FMT_MSC_DEFAULT;
};

/**
  \rst
  Constructs a `~fmt::format_arg_store` object that contains references
  to arguments and can be implicitly converted to `~fmt::format_args`.
  If ``fmt`` is a compile-time string then `make_args_checked` checks
  its validity at compile time.
  \endrst
 */
template <typename... Args, typename S, typename Char = char_t<S>>
FMT_INLINE auto make_args_checked(const S& fmt,
                                  const remove_reference_t<Args>&... args)
    -> format_arg_store<buffer_context<Char>, remove_reference_t<Args>...> {
  static_assert(
      detail::count<(
              std::is_base_of<detail::view, remove_reference_t<Args>>::value &&
              std::is_reference<Args>::value)...>() == 0,
      "passing views as lvalues is disallowed");
  detail::check_format_string<Args...>(fmt);
  return {args...};
}

// compile-time support
namespace detail_exported {
#if FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
template <typename Char, size_t N> struct fixed_string {
  constexpr fixed_string(const Char (&str)[N]) {
    detail::copy_str<Char, const Char*, Char*>(static_cast<const Char*>(str),
                                               str + N, data);
  }
  Char data[N]{};
};
#endif

// Converts a compile-time string to basic_string_view.
template <typename Char, size_t N>
constexpr auto compile_string_to_view(const Char (&s)[N])
    -> basic_string_view<Char> {
  // Remove trailing NUL character if needed. Won't be present if this is used
  // with a raw character array (i.e. not defined as a string).
  return {s, N - (std::char_traits<Char>::to_int_type(s[N - 1]) == 0 ? 1 : 0)};
}
template <typename Char>
constexpr auto compile_string_to_view(detail::std_string_view<Char> s)
    -> basic_string_view<Char> {
  return {s.data(), s.size()};
}
}  // namespace detail_exported

FMT_BEGIN_DETAIL_NAMESPACE

template <typename Char>
void vformat_to(
    buffer<Char>& buf, basic_string_view<Char> fmt,
    basic_format_args<FMT_BUFFER_CONTEXT(type_identity_t<Char>)> args,
    locale_ref loc = {});

FMT_API void vprint_mojibake(std::FILE*, string_view, format_args);
#ifndef _WIN32
inline void vprint_mojibake(std::FILE*, string_view, format_args) {}
#endif
FMT_END_DETAIL_NAMESPACE

FMT_API auto vformat(string_view fmt, format_args args) -> std::string;

/**
  \rst
  Formats ``args`` according to specifications in ``fmt`` and returns the result
  as a string.

  **Example**::

    #include <fmt/core.h>
    std::string message = fmt::format("The answer is {}.", 42);
  \endrst
*/
template <typename... T>
FMT_NODISCARD FMT_INLINE auto format(format_string<T...> fmt, T&&... args)
    -> std::string {
  return vformat(fmt, fmt::make_format_args(args...));
}

/** Formats a string and writes the output to ``out``. */
template <typename OutputIt,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
auto vformat_to(OutputIt out, string_view fmt, format_args args) -> OutputIt {
  using detail::get_buffer;
  auto&& buf = get_buffer<char>(out);
  detail::vformat_to(buf, fmt, args, {});
  return detail::get_iterator(buf);
}

/**
 \rst
 Formats ``args`` according to specifications in ``fmt``, writes the result to
 the output iterator ``out`` and returns the iterator past the end of the output
 range. `format_to` does not append a terminating null character.

 **Example**::

   auto out = std::vector<char>();
   fmt::format_to(std::back_inserter(out), "{}", 42);
 \endrst
 */
template <typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
FMT_INLINE auto format_to(OutputIt out, format_string<T...> fmt, T&&... args)
    -> OutputIt {
  return vformat_to(out, fmt, fmt::make_format_args(args...));
}

template <typename OutputIt> struct format_to_n_result {
  /** Iterator past the end of the output range. */
  OutputIt out;
  /** Total (not truncated) output size. */
  size_t size;
};

template <typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
auto vformat_to_n(OutputIt out, size_t n, string_view fmt, format_args args)
    -> format_to_n_result<OutputIt> {
  using traits = detail::fixed_buffer_traits;
  auto buf = detail::iterator_buffer<OutputIt, char, traits>(out, n);
  detail::vformat_to(buf, fmt, args, {});
  return {buf.out(), buf.count()};
}

/**
  \rst
  Formats ``args`` according to specifications in ``fmt``, writes up to ``n``
  characters of the result to the output iterator ``out`` and returns the total
  (not truncated) output size and the iterator past the end of the output range.
  `format_to_n` does not append a terminating null character.
  \endrst
 */
template <typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
FMT_INLINE auto format_to_n(OutputIt out, size_t n, format_string<T...> fmt,
                            T&&... args) -> format_to_n_result<OutputIt> {
  return vformat_to_n(out, n, fmt, fmt::make_format_args(args...));
}

/** Returns the number of chars in the output of ``format(fmt, args...)``. */
template <typename... T>
FMT_NODISCARD FMT_INLINE auto formatted_size(format_string<T...> fmt,
                                             T&&... args) -> size_t {
  auto buf = detail::counting_buffer<>();
  detail::vformat_to(buf, string_view(fmt), fmt::make_format_args(args...), {});
  return buf.count();
}

FMT_API void vprint(string_view fmt, format_args args);
FMT_API void vprint(std::FILE* f, string_view fmt, format_args args);

/**
  \rst
  Formats ``args`` according to specifications in ``fmt`` and writes the output
  to ``stdout``.

  **Example**::

    fmt::print("Elapsed time: {0:.2f} seconds", 1.23);
  \endrst
 */
template <typename... T>
FMT_INLINE void print(format_string<T...> fmt, T&&... args) {
  const auto& vargs = fmt::make_format_args(args...);
  return detail::is_utf8() ? vprint(fmt, vargs)
                           : detail::vprint_mojibake(stdout, fmt, vargs);
}

/**
  \rst
  Formats ``args`` according to specifications in ``fmt`` and writes the
  output to the file ``f``.

  **Example**::

    fmt::print(stderr, "Don't {}!", "panic");
  \endrst
 */
template <typename... T>
FMT_INLINE void print(std::FILE* f, format_string<T...> fmt, T&&... args) {
  const auto& vargs = fmt::make_format_args(args...);
  return detail::is_utf8() ? vprint(f, fmt, vargs)
                           : detail::vprint_mojibake(f, fmt, vargs);
}

FMT_MODULE_EXPORT_END
FMT_GCC_PRAGMA("GCC pop_options")
FMT_END_NAMESPACE

#ifdef FMT_HEADER_ONLY
#  define FMT_FUNC inline
#  include "format-inl.h"
#else
#  define FMT_FUNC
#endif

#endif  // FMT_FORMAT_H_
