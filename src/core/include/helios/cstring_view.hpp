#pragma once

#include <helios/assert.hpp>

#include <algorithm>
#include <compare>
#include <cstddef>
#include <format>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace helios {

/**
 * @brief A view of a null-terminated C string.
 * @tparam CharT Character type
 * @tparam Traits Character traits type
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class BasicCStringView {
public:
  using traits_type = Traits;
  using value_type = CharT;
  using pointer = const CharT*;
  using const_pointer = const CharT*;
  using reference = const CharT&;
  using const_reference = const CharT&;
  using iterator = const CharT*;
  using const_iterator = const CharT*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  static constexpr size_type npos = static_cast<size_type>(-1);

  BasicCStringView() = delete;

  /**
   * @brief Constructs from a string.
   * @param string Source string
   */
  constexpr explicit BasicCStringView(
      const std::basic_string<CharT, Traits>& string) noexcept
      : size_(string.size()), data_(string.data()) {}

  /**
   * @brief Constructs from a null-terminated C string.
   * @param str Source C string
   */
  constexpr BasicCStringView(const CharT* str) noexcept
      : size_(std::char_traits<CharT>::length(str)), data_(str) {}

  BasicCStringView(std::nullptr_t) = delete;
  constexpr BasicCStringView(const BasicCStringView&) noexcept = default;
  constexpr BasicCStringView(BasicCStringView&&) noexcept = default;
  constexpr ~BasicCStringView() noexcept = default;

  constexpr BasicCStringView& operator=(const BasicCStringView&) noexcept =
      default;
  constexpr BasicCStringView& operator=(BasicCStringView&&) noexcept = default;

  /**
   * @brief Assigns a `std::basic_string`.
   * @param string String to assign
   * @return Reference to this
   */
  constexpr BasicCStringView& Assign(
      const std::basic_string<CharT, Traits>& string) noexcept;

  /**
   * @brief Assigns a null-terminated C string.
   * @param str C string to assign
   * @return Reference to this
   */
  constexpr BasicCStringView& Assign(const CharT* str) noexcept;

  /**
   * @brief Copies characters to a buffer.
   * @warning Triggers assertion if `pos > Size()`.
   * @param dest Destination buffer
   * @param count Maximum number of characters to copy
   * @param pos Starting position
   * @return Number of characters copied
   */
  constexpr size_type Copy(CharT* dest, size_type count,
                           size_type pos = 0) const noexcept;

  /**
   * @brief Swaps contents with another `BasicCStringView`.
   * @param other String to swap with
   */
  constexpr void Swap(BasicCStringView& other) noexcept;
  friend constexpr void swap(BasicCStringView& lhs,
                             BasicCStringView& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Compares with another string.
   * @param other String to compare with
   * @return Negative if less, zero if equal, positive if greater
   */
  [[nodiscard]] constexpr int Compare(
      std::basic_string_view<CharT, Traits> other) const noexcept {
    return View().compare(other);
  }

  /**
   * @brief Compares with another c string view.
   * @param other C string view to compare with
   * @return Negative if less, zero if equal, positive if greater
   */
  [[nodiscard]] constexpr int Compare(
      BasicCStringView<CharT, Traits> other) const noexcept {
    return View().compare(other.View());
  }

  /**
   * @brief Finds first occurrence of substring.
   * @param sv Substring to search for
   * @param pos Starting position
   * @return Position of first occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type Find(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = 0) const noexcept {
    return View().find(sv, pos);
  }

  /**
   * @brief Finds first occurrence of character.
   * @param ch Character to search for
   * @param pos Starting position
   * @return Position of first occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type Find(CharT ch,
                                         size_type pos = 0) const noexcept {
    return View().find(ch, pos);
  }

  /**
   * @brief Finds last occurrence of substring.
   * @param sv Substring to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type RFind(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = npos) const noexcept {
    return View().rfind(sv, pos);
  }

  /**
   * @brief Finds last occurrence of character.
   * @param ch Character to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type RFind(CharT ch,
                                          size_type pos = npos) const noexcept {
    return View().rfind(ch, pos);
  }

  /**
   * @brief Finds first occurrence of any character in sv.
   * @param sv Characters to search for
   * @param pos Starting position
   * @return Position of first occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type FindFirstOf(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = 0) const noexcept {
    return View().find_first_of(sv, pos);
  }

  /**
   * @brief Finds last occurrence of any character in sv.
   * @param sv Characters to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type FindLastOf(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = npos) const noexcept {
    return View().find_last_of(sv, pos);
  }

  /**
   * @brief Finds first character not in sv.
   * @param sv Characters to exclude
   * @param pos Starting position
   * @return Position of first non-matching character, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type FindFirstNotOf(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = 0) const noexcept {
    return View().find_first_not_of(sv, pos);
  }

  /**
   * @brief Finds last character not in sv.
   * @param sv Characters to exclude
   * @param pos Starting position (searches backwards from here)
   * @return Position of last non-matching character, or `npos` if not found
   */
  [[nodiscard]] constexpr size_type FindLastNotOf(
      std::basic_string_view<CharT, Traits> sv,
      size_type pos = npos) const noexcept {
    return View().find_last_not_of(sv, pos);
  }

  /**
   * @brief Accesses character at specified position.
   * @warning Triggers assertion if `pos >= Size()`.
   * @param pos Position of the character
   * @return Reference to the character
   */
  [[nodiscard]] constexpr reference operator[](size_type pos) noexcept;

  /**
   * @brief Accesses character at specified position (const).
   * @warning Triggers assertion if `pos >= Size()`.
   * @param pos Position of the character
   * @return Const reference to the character
   */
  [[nodiscard]] constexpr const_reference operator[](
      size_type pos) const noexcept;

  /**
   * @brief Assigns a `std::basic_string`.
   * @param string String to assign
   * @return Reference to this
   */
  constexpr BasicCStringView& operator=(
      const std::basic_string<CharT, Traits>& string) noexcept;

  /**
   * @brief Assigns a null-terminated C string.
   * @param str C string to assign
   * @return Reference to this
   */
  constexpr BasicCStringView& operator=(const CharT* str) noexcept;

  /**
   * @brief Converts to `std::basic_string_view`.
   * @return String view of the content
   */
  [[nodiscard]] constexpr operator std::basic_string_view<CharT, Traits>()
      const noexcept {
    return std::basic_string_view<CharT, Traits>(data_, size_);
  }

  [[nodiscard]] constexpr auto operator<=>(
      BasicCStringView<CharT, Traits> other) const noexcept
      -> std::strong_ordering;

  [[nodiscard]] constexpr auto operator<=>(
      std::basic_string_view<CharT, Traits> other) const noexcept
      -> std::strong_ordering;

  /**
   * @brief Checks if the string is empty.
   * @return True if `Size() == 0`
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return size_ == 0; }

  /**
   * @brief Checks if string starts with prefix.
   * @param sv Prefix to check
   * @return True if string starts with sv
   */
  [[nodiscard]] constexpr bool StartsWith(
      std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().starts_with(sv);
  }

  /**
   * @brief Checks if string starts with character.
   * @param ch Character to check
   * @return True if string starts with ch
   */
  [[nodiscard]] constexpr bool StartsWith(CharT ch) const noexcept {
    return !Empty() && Front() == ch;
  }

  /**
   * @brief Checks if string ends with suffix.
   * @param sv Suffix to check
   * @return True if string ends with sv
   */
  [[nodiscard]] constexpr bool EndsWith(
      std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().ends_with(sv);
  }

  /**
   * @brief Checks if string ends with character.
   * @param ch Character to check
   * @return True if string ends with ch
   */
  [[nodiscard]] constexpr bool EndsWith(CharT ch) const noexcept {
    return !Empty() && Back() == ch;
  }

  /**
   * @brief Checks if string contains substring.
   * @param sv Substring to search for
   * @return True if string contains sv
   */
  [[nodiscard]] constexpr bool Contains(
      std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().find(sv) != npos;
  }

  /**
   * @brief Checks if string contains character.
   * @param ch Character to search for
   * @return True if string contains ch
   */
  [[nodiscard]] constexpr bool Contains(CharT ch) const noexcept {
    return View().find(ch) != npos;
  }

  /**
   * @brief Returns the number of characters.
   * @return Number of characters (excluding null terminator)
   */
  [[nodiscard]] constexpr size_type Size() const noexcept { return size_; }

  /**
   * @brief Returns the number of characters.
   * @return Number of characters (excluding null terminator)
   */
  [[nodiscard]] constexpr size_type Length() const noexcept { return size_; }

  /**
   * @brief Accesses character at specified position with bounds checking.
   * @warning Triggers assertion if `pos >= Size()`.
   * @param pos Position of the character
   * @return Reference to the character
   */
  [[nodiscard]] constexpr reference At(size_type pos) noexcept;

  /**
   * @brief Accesses character at specified position with bounds checking
   * (const).
   * @warning Triggers assertion if `pos >= Size()`.
   * @param pos Position of the character
   * @return Const reference to the character
   *
   */
  [[nodiscard]] constexpr const_reference At(size_type pos) const noexcept;

  /**
   * @brief Accesses the first character.
   * @warning Triggers assertion if string is empty.
   * @return Reference to the first character
   */
  [[nodiscard]] constexpr reference Front() noexcept;

  /**
   * @brief Accesses the first character (const).
   * @warning Triggers assertion if string is empty.
   * @return Const reference to the first character
   */
  [[nodiscard]] constexpr const_reference Front() const noexcept;

  /**
   * @brief Accesses the last character.
   * @warning Triggers assertion if string is empty.
   * @return Reference to the last character
   */
  [[nodiscard]] constexpr reference Back() noexcept;

  /**
   * @brief Accesses the last character (const).
   * @warning Triggers assertion if string is empty.
   * @return Const reference to the last character
   */
  [[nodiscard]] constexpr const_reference Back() const noexcept;

  /**
   * @brief Returns pointer to the underlying character array.
   * @return Pointer to the character array
   */
  [[nodiscard]] constexpr pointer Data() noexcept { return data_; }

  /**
   * @brief Returns pointer to the underlying character array (const).
   * @return Const pointer to the character array
   */
  [[nodiscard]] constexpr const_pointer Data() const noexcept { return data_; }

  /**
   * @brief Returns null-terminated C string.
   * @return Null-terminated character array
   */
  [[nodiscard]] constexpr const_pointer CStr() const noexcept { return data_; }

  /**
   * @brief Returns a `std::basic_string_view` of the content.
   * @return String view of the content
   */
  [[nodiscard]] constexpr std::basic_string_view<CharT, Traits> View()
      const noexcept {
    return std::basic_string_view<CharT, Traits>(data_, size_);
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return data_; }
  [[nodiscard]] constexpr const_iterator begin() const noexcept {
    return data_;
  }

  [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
    return data_;
  }

  [[nodiscard]] constexpr iterator end() noexcept { return data_ + size_; }
  [[nodiscard]] constexpr const_iterator end() const noexcept {
    return data_ + size_;
  }

  [[nodiscard]] constexpr const_iterator cend() const noexcept {
    return data_ + size_;
  }

  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
    return reverse_iterator(end());
  }

  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] constexpr reverse_iterator rend() noexcept {
    return reverse_iterator(begin());
  }

  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
    return const_reverse_iterator(begin());
  }

private:
  size_t size_ = 0;
  const CharT* data_ = nullptr;
};

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Assign(
    const std::basic_string<CharT, Traits>& string) noexcept
    -> BasicCStringView& {
  size_ = string.size();
  data_ = string.data();
  return *this;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Assign(
    const CharT* str) noexcept -> BasicCStringView& {
  size_ = std::char_traits<CharT>::length(str);
  data_ = str;
  return *this;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Copy(
    CharT* dest, size_type count, size_type pos) const noexcept -> size_type {
  HELIOS_ASSERT(pos < Size(), "pos out of bounds!");
  const size_type copy_count = std::min(count, Size() - pos);
  std::copy_n(data_ + pos, copy_count, dest);
  return copy_count;
}

template <typename CharT, typename Traits>
constexpr void BasicCStringView<CharT, Traits>::Swap(
    BasicCStringView& other) noexcept {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator[](
    size_type pos) noexcept -> reference {
  HELIOS_ASSERT(pos < Size(), "index out of bounds!");
  return data_[pos];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator[](
    size_type pos) const noexcept -> const_reference {
  HELIOS_ASSERT(pos < Size(), "index out of bounds!");
  return data_[pos];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator=(
    const std::basic_string<CharT, Traits>& string) noexcept
    -> BasicCStringView& {
  Assign(string);
  return *this;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator=(
    const CharT* str) noexcept -> BasicCStringView& {
  Assign(str);
  return *this;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator<=>(
    BasicCStringView<CharT, Traits> other) const noexcept
    -> std::strong_ordering {
  const auto result = Compare(other);
  if (result < 0) {
    return std::strong_ordering::less;
  }
  if (result > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::operator<=>(
    std::basic_string_view<CharT, Traits> other) const noexcept
    -> std::strong_ordering {
  const auto result = Compare(other);
  if (result < 0) {
    return std::strong_ordering::less;
  }
  if (result > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::At(size_type pos) noexcept
    -> reference {
  HELIOS_ASSERT(pos < Size(), "pos out of bounds!");
  return data_[pos];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::At(size_type pos) const noexcept
    -> const_reference {
  HELIOS_ASSERT(pos < Size(), "pos out of bounds!");
  return data_[pos];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Front() noexcept -> reference {
  HELIOS_ASSERT(!Empty(), "string is empty");
  return data_[0];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Front() const noexcept
    -> const_reference {
  HELIOS_ASSERT(!Empty(), "string is empty!");
  return data_[0];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Back() noexcept -> reference {
  HELIOS_ASSERT(!Empty(), "string is empty!");
  return data_[Size() - 1];
}

template <typename CharT, typename Traits>
constexpr auto BasicCStringView<CharT, Traits>::Back() const noexcept
    -> const_reference {
  HELIOS_ASSERT(!Empty(), "string is empty!");
  return data_[Size() - 1];
}

/// @brief A view of a null-terminated C string.
using CStringView = BasicCStringView<char>;

/// @brief A view of a null-terminated wide C string.
using WCStringView = BasicCStringView<wchar_t>;

/// @brief A view of a null-terminated UTF-8 C string.
using U8CStringView = BasicCStringView<char8_t>;

/// @brief A view of a null-terminated UTF-16 C string.
using U16CStringView = BasicCStringView<char16_t>;

/// @brief A view of a null-terminated UTF-32 C string.
using U32CStringView = BasicCStringView<char32_t>;

/**
 * @brief Outputs a `BasicCStringView` to an output stream.
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param os Output stream
 * @param str String to output
 * @return Reference to the output stream
 */
template <typename CharT, typename Traits>
inline auto operator<<(std::basic_ostream<CharT, Traits>& os,
                       const BasicCStringView<CharT, Traits>& str)
    -> std::basic_ostream<CharT, Traits>& {
  return os << str.View();
}

}  // namespace helios

namespace std {

/**
 * @brief Hash support for `BasicCStringView`.
 * @tparam CharT Character type
 * @tparam Traits Character traits
 */
template <typename CharT, typename Traits>
struct hash<helios::BasicCStringView<CharT, Traits>> {
  [[nodiscard]] constexpr size_t operator()(
      const helios::BasicCStringView<CharT, Traits>& str) const noexcept {
    return hash<basic_string_view<CharT, Traits>>{}(str.View());
  }
};

/**
 * @brief `std::format` support for `BasicCStringView`.
 * @details Enables formatting `BasicCStringView` using `std::format`
 * by delegating to `std::basic_string_view` formatter.
 * @tparam CharT Character type
 * @tparam Traits Character traits
 */
template <typename CharT, typename Traits>
struct formatter<helios::BasicCStringView<CharT, Traits>>
    : formatter<basic_string_view<CharT, Traits>> {
  /// @brief Format the `BasicCStringView` by delegating to
  /// `std::basic_string_view` formatter.
  auto format(const helios::BasicCStringView<CharT, Traits>& str,
              format_context& ctx) const {
    return formatter<basic_string_view<CharT, Traits>>::format(str.View(), ctx);
  }
};

}  // namespace std
