#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/assert.hpp>

#include <algorithm>
#include <array>
#include <compare>
#include <cstddef>
#include <format>
#include <functional>
#include <istream>
#include <iterator>
#include <ostream>
#include <ranges>
#include <string>
#include <string_view>

namespace helios::container {

/**
 * @brief A fixed-capacity string class that owns its storage.
 * @details Similar to std::string_view but mutable and owning, with a compile-time fixed capacity.
 * Provides all operations of std::string_view plus mutation operations.
 * Does not allocate heap memory.
 *
 * @tparam StrCapacity Maximum number of characters (excluding null terminator)
 * @tparam CharT Character type
 * @tparam Traits Character traits type
 */
template <size_t StrCapacity, typename CharT, typename Traits = std::char_traits<CharT>>
  requires(StrCapacity > 0)
class BasicStaticString final {
public:
  // Type aliases (STL compatible)
  using traits_type = Traits;
  using value_type = CharT;
  using pointer = CharT*;
  using const_pointer = const CharT*;
  using reference = CharT&;
  using const_reference = const CharT&;
  using iterator = CharT*;
  using const_iterator = const CharT*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  static constexpr size_type npos = static_cast<size_type>(-1);

  /**
   * @brief Default constructor. Creates an empty string.
   */
  constexpr BasicStaticString() noexcept { data_[0] = CharT{}; }

  /**
   * @brief Constructs from a string_view.
   * @warning Triggers assertion if sv.size() > StrCapacity.
   * @param sv Source string view
   */
  constexpr explicit BasicStaticString(std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Constructs a substring from a string_view with position and count.
   * @warning Triggers assertion if (pos + count) > sv.size() or count > StrCapacity.
   * @param sv Source string view
   * @param pos Starting position in the view
   * @param count Number of characters to copy (default: all remaining)
   */
  constexpr explicit BasicStaticString(std::basic_string_view<CharT, Traits> sv, size_type pos,
                                       size_type count = npos) noexcept;

  /**
   * @brief Constructs from a null-terminated C string.
   * @warning Triggers assertion if string length > StrCapacity.
   * @param str Source C string
   */
  constexpr explicit BasicStaticString(const CharT* str) noexcept
      : BasicStaticString(std::basic_string_view<CharT, Traits>(str)) {}

  /**
   * @brief Constructs from a character array of known size.
   * @warning Triggers assertion if N-1 > StrCapacity (excluding null terminator).
   * @tparam N Size of the character array
   * @param str Source character array
   */
  template <size_t N>
    requires(N <= StrCapacity + 1)
  constexpr explicit(false) BasicStaticString(const CharT (&str)[N]) noexcept;

  /**
   * @brief Constructs from count copies of character ch.
   * @warning Triggers assertion if count > StrCapacity.
   * @param count Number of characters
   * @param ch Character to repeat
   */
  constexpr BasicStaticString(size_type count, CharT ch) noexcept;

  /**
   * @brief Constructs from a pointer and length.
   * @warning Triggers assertion if len > StrCapacity.
   * @param str Source string pointer
   * @param len Length of the string
   */
  constexpr BasicStaticString(const CharT* str, size_type len) noexcept;

  /**
   * @brief Constructs from an iterator range.
   * @warning Triggers assertion if distance(first, last) > StrCapacity.
   * @tparam InputIt Input iterator type
   * @param first Beginning of range
   * @param last End of range
   */
  template <std::input_iterator InputIt>
  constexpr BasicStaticString(InputIt first, InputIt last) noexcept;

  /**
   * @brief Constructs from a range using std::from_range_t (C++23).
   * @warning Triggers assertion if range size > StrCapacity.
   * @tparam Range Range type
   * @param rg Range to construct from
   */
  template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr BasicStaticString(std::from_range_t, Range&& rg) noexcept;

  /**
   * @brief Constructs from an initializer list.
   * @warning Triggers assertion if ilist.size() > StrCapacity.
   * @param ilist Initializer list
   */
  constexpr BasicStaticString(std::initializer_list<CharT> ilist) noexcept;

  /**
   * @brief Explicitly delete construction from nullptr.
   */
  BasicStaticString(std::nullptr_t) = delete;

  constexpr BasicStaticString(const BasicStaticString&) noexcept = default;
  constexpr BasicStaticString(BasicStaticString&&) noexcept = default;

  /**
   * @brief Copy constructor from smaller capacity string.
   * @warning Triggers assertion if other.Size() > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr explicit BasicStaticString(const BasicStaticString<OtherCapacity, CharT, Traits>& other) noexcept;

  /**
   * @brief Move constructor from smaller capacity string.
   * @warning Triggers assertion if other.Size() > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr explicit BasicStaticString(BasicStaticString<OtherCapacity, CharT, Traits>&& other) noexcept;

  /**
   * @brief Substring copy constructor from same capacity string.
   * @warning Triggers assertion if (pos + count) > other.Size() or count > StrCapacity.
   * @param other Source string
   * @param pos Starting position (default: 0)
   * @param count Number of characters to copy (default: all remaining)
   */
  constexpr BasicStaticString(const BasicStaticString& other, size_type pos, size_type count = npos) noexcept;

  /**
   * @brief Substring move constructor from same capacity string (C++23).
   * @warning Triggers assertion if (pos + count) > other.Size() or count > StrCapacity.
   * @param other Source string
   * @param pos Starting position (default: 0)
   * @param count Number of characters to copy (default: all remaining)
   */
  constexpr BasicStaticString(BasicStaticString&& other, size_type pos, size_type count = npos) noexcept;

  /**
   * @brief Substring copy constructor from smaller capacity string.
   * @warning Triggers assertion if (pos + count) > other.Size() or count > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   * @param pos Starting position (default: 0)
   * @param count Number of characters to copy (default: all remaining)
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr BasicStaticString(const BasicStaticString<OtherCapacity, CharT, Traits>& other, size_type pos,
                              size_type count = npos) noexcept;

  /**
   * @brief Substring move constructor from smaller capacity string (C++23).
   * @warning Triggers assertion if (pos + count) > other.Size() or count > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   * @param pos Starting position (default: 0)
   * @param count Number of characters to copy (default: all remaining)
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr BasicStaticString(BasicStaticString<OtherCapacity, CharT, Traits>&& other, size_type pos,
                              size_type count = npos) noexcept;

  constexpr ~BasicStaticString() noexcept = default;

  constexpr BasicStaticString& operator=(const BasicStaticString&) noexcept = default;
  constexpr BasicStaticString& operator=(BasicStaticString&&) noexcept = default;

  /**
   * @brief Copy assignment from smaller capacity string.
   * @warning Triggers assertion if other.Size() > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   * @return Reference to this
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr BasicStaticString& operator=(const BasicStaticString<OtherCapacity, CharT, Traits>& other) noexcept;

  /**
   * @brief Move assignment from smaller capacity string.
   * @warning Triggers assertion if other.Size() > StrCapacity.
   * @tparam OtherCapacity StrCapacity of source string (must be less than StrCapacity)
   * @param other Source string with smaller capacity
   * @return Reference to this
   */
  template <size_t OtherCapacity>
    requires(OtherCapacity < StrCapacity)
  constexpr BasicStaticString& operator=(BasicStaticString<OtherCapacity, CharT, Traits>&& other) noexcept;

  /**
   * @brief Assigns from a string_view.
   * @warning Triggers assertion if sv.size() > StrCapacity.
   * @param sv Source string view
   * @return Reference to this
   */
  constexpr BasicStaticString& operator=(std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Assigns from a null-terminated C string.
   * @warning Triggers assertion if string length > StrCapacity.
   * @param str Source C string
   * @return Reference to this
   */
  constexpr BasicStaticString& operator=(const CharT* str) noexcept;

  /**
   * @brief Clears the string content.
   */
  constexpr void Clear() noexcept;

  /**
   * @brief Inserts a string_view at position.
   * @warning Triggers assertion if pos > Size() or resulting size > capacity().
   * @param pos Position to insert at
   * @param sv String view to insert
   * @return Reference to this
   */
  constexpr BasicStaticString& Insert(size_type pos, std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Inserts count copies of character at position.\
   * @warning Triggers assertion if pos > Size() or resulting size > capacity()
   * @param pos Position to insert at
   * @param count Number of characters
   * @param ch Character to insert
   * @return Reference to this
   */
  constexpr BasicStaticString& Insert(size_type pos, size_type count, CharT ch) noexcept;

  /**
   * @brief Inserts characters from a range at position.
   * @warning Triggers assertion if pos > Size() or resulting size > capacity().
   * @tparam Range Range type
   * @param pos Position to insert at
   * @param rg Range to insert
   * @return Reference to this
   */
  template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr BasicStaticString& InsertRange(size_type pos, Range&& rg) noexcept;

  /**
   * @brief Erases characters from pos to pos + count.
   * @warning Triggers assertion if pos > Size().
   * @param pos Starting position
   * @param count Number of characters to erase (default: all remaining)
   * @return Reference to this
   */
  constexpr BasicStaticString& Erase(size_type pos = 0, size_type count = npos) noexcept;

  /**
   * @brief Appends a character.
   * @warning Triggers assertion if Size() >= capacity().
   * @param ch Character to append
   */
  constexpr void PushBack(CharT ch) noexcept;

  /**
   * @brief Removes the last character.
   * @warning Triggers assertion if string is empty.
   */
  constexpr void PopBack() noexcept;

  /**
   * @brief Appends a string_view.
   * @warning Triggers assertion if resulting size > capacity().
   * @param sv String view to append
   * @return Reference to this
   */
  constexpr BasicStaticString& Append(std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Appends count copies of character ch.
   * @warning Triggers assertion if resulting size > capacity().
   * @param count Number of characters
   * @param ch Character to append
   * @return Reference to this
   */
  constexpr BasicStaticString& Append(size_type count, CharT ch) noexcept;

  /**
   * @brief Appends a null-terminated C string.
   * @warning Triggers assertion if resulting size > capacity().
   * @param str C string to append
   * @return Reference to this
   */
  constexpr BasicStaticString& Append(const CharT* str) noexcept {
    return Append(std::basic_string_view<CharT, Traits>(str));
  }

  /**
   * @brief Appends characters from a range.
   * @warning Triggers assertion if resulting size > capacity().
   * @tparam Range Range type
   * @param rg Range to append
   * @return Reference to this
   */
  template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr BasicStaticString& AppendRange(Range&& rg) noexcept;

  /**
   * @brief Replaces characters in range [pos, pos + count) with string_view.
   * @warning Triggers assertion if pos > Size() or resulting size > capacity().
   * @param pos Starting position
   * @param count Number of characters to replace
   * @param sv Replacement string view
   * @return Reference to this
   */
  constexpr BasicStaticString& Replace(size_type pos, size_type count,
                                       std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Replaces characters in range [pos, pos + count) with count2 copies of ch.
   * @warning Triggers assertion if pos > Size() or resulting size > capacity().
   * @param pos Starting position
   * @param count Number of characters to replace
   * @param count2 Number of replacement characters
   * @param ch Replacement character
   * @return Reference to this
   */
  constexpr BasicStaticString& Replace(size_type pos, size_type count, size_type count2, CharT ch) noexcept;

  /**
   * @brief Replaces characters in range [pos, pos + count) with characters from a range.
   * @warning Triggers assertion if pos > Size() or resulting size > capacity().
   * @tparam Range Range type
   * @param pos Starting position
   * @param count Number of characters to replace
   * @param rg Range of replacement characters
   * @return Reference to this
   */
  template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr BasicStaticString& ReplaceWithRange(size_type pos, size_type count, Range&& rg) noexcept;

  /**
   * @brief Copies characters to a buffer.
   * @warning Triggers assertion if pos > Size().
   * @param dest Destination buffer
   * @param count Maximum number of characters to copy
   * @param pos Starting position
   * @return Number of characters copied
   */
  constexpr size_type Copy(CharT* dest, size_type count, size_type pos = 0) const noexcept;

  /**
   * @brief Resizes the string.
   * @warning Triggers assertion if count > capacity()
   * @param count New size
   * @param ch Character to fill if expanding
   */
  constexpr void Resize(size_type count, CharT ch = CharT{}) noexcept;

  /**
   * @brief Swaps contents with another BasicStaticString.
   * @param other String to swap with
   */
  constexpr void Swap(BasicStaticString& other) noexcept;

  /**
   * @brief Assigns a string_view.
   * @warning Triggers assertion if sv.size() > capacity().
   * @param sv String view to assign
   * @return Reference to this
   */
  constexpr BasicStaticString& Assign(std::basic_string_view<CharT, Traits> sv) noexcept;

  /**
   * @brief Assigns a null-terminated C string.
   * @warning Triggers assertion if string length > capacity().
   * @param str C string to assign
   * @return Reference to this
   */
  constexpr BasicStaticString& Assign(const CharT* str) noexcept {
    return Assign(std::basic_string_view<CharT, Traits>(str));
  }

  /**
   * @brief Assigns count copies of character ch.
   * @warning Triggers assertion if count > capacity().
   * @param count Number of characters
   * @param ch Character to assign
   * @return Reference to this
   */
  constexpr BasicStaticString& Assign(size_type count, CharT ch) noexcept;

  /**
   * @brief Assigns characters from a range.
   * @warning Triggers assertion if range size > capacity().
   * @tparam Range Range type
   * @param rg Range to assign from
   * @return Reference to this
   */
  template <std::ranges::input_range Range>
    requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
  constexpr BasicStaticString& AssignRange(Range&& rg) noexcept;

  /**
   * @brief Returns a substring.
   * @warning Triggers assertion if pos > Size().
   * @param pos Starting position
   * @param count Number of characters (default: all remaining)
   * @return Substring as StaticString with same capacity
   */
  [[nodiscard]] constexpr BasicStaticString Substr(size_type pos = 0, size_type count = npos) const noexcept;

  /**
   * @brief Compares with another string.
   * @param other String to compare with
   * @return Negative if less, zero if equal, positive if greater
   */
  [[nodiscard]] constexpr int Compare(std::basic_string_view<CharT, Traits> other) const noexcept {
    return View().compare(other);
  }

  /**
   * @brief Compares with another BasicStaticString.
   * @tparam OtherCapacity StrCapacity of the other string
   * @param other String to compare with
   * @return Negative if less, zero if equal, positive if greater
   */
  template <size_t OtherCapacity>
  [[nodiscard]] constexpr int Compare(const BasicStaticString<OtherCapacity, CharT, Traits>& other) const noexcept {
    return View().compare(other.View());
  }

  /**
   * @brief Finds first occurrence of substring.
   * @param sv Substring to search for
   * @param pos Starting position
   * @return Position of first occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type Find(std::basic_string_view<CharT, Traits> sv, size_type pos = 0) const noexcept {
    return View().find(sv, pos);
  }

  /**
   * @brief Finds first occurrence of character.
   * @param ch Character to search for
   * @param pos Starting position
   * @return Position of first occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type Find(CharT ch, size_type pos = 0) const noexcept { return View().find(ch, pos); }

  /**
   * @brief Finds last occurrence of substring.
   * @param sv Substring to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type RFind(std::basic_string_view<CharT, Traits> sv,
                                          size_type pos = npos) const noexcept {
    return View().rfind(sv, pos);
  }

  /**
   * @brief Finds last occurrence of character.
   * @param ch Character to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type RFind(CharT ch, size_type pos = npos) const noexcept {
    return View().rfind(ch, pos);
  }

  /**
   * @brief Finds first occurrence of any character in sv.
   * @param sv Characters to search for
   * @param pos Starting position
   * @return Position of first occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type FindFirstOf(std::basic_string_view<CharT, Traits> sv,
                                                size_type pos = 0) const noexcept {
    return View().find_first_of(sv, pos);
  }

  /**
   * @brief Finds last occurrence of any character in sv.
   * @param sv Characters to search for
   * @param pos Starting position (searches backwards from here)
   * @return Position of last occurrence, or npos if not found
   */
  [[nodiscard]] constexpr size_type FindLastOf(std::basic_string_view<CharT, Traits> sv,
                                               size_type pos = npos) const noexcept {
    return View().find_last_of(sv, pos);
  }

  /**
   * @brief Finds first character not in sv.
   * @param sv Characters to exclude
   * @param pos Starting position
   * @return Position of first non-matching character, or npos if not found
   */
  [[nodiscard]] constexpr size_type FindFirstNotOf(std::basic_string_view<CharT, Traits> sv,
                                                   size_type pos = 0) const noexcept {
    return View().find_first_not_of(sv, pos);
  }

  /**
   * @brief Finds last character not in sv.
   * @param sv Characters to exclude
   * @param pos Starting position (searches backwards from here)
   * @return Position of last non-matching character, or npos if not found
   */
  [[nodiscard]] constexpr size_type FindLastNotOf(std::basic_string_view<CharT, Traits> sv,
                                                  size_type pos = npos) const noexcept {
    return View().find_last_not_of(sv, pos);
  }

  /**
   * @brief Appends a string_view.
   * @warning Triggers assertion if resulting size > capacity().
   * @param sv String view to append
   * @return Reference to this
   */
  constexpr BasicStaticString& operator+=(std::basic_string_view<CharT, Traits> sv) noexcept { return Append(sv); }

  /**
   * @brief Appends a character.
   * @warning Triggers assertion if resulting size > capacity().
   * @param ch Character to append
   * @return Reference to this
   */
  constexpr BasicStaticString& operator+=(CharT ch) noexcept;

  /**
   * @brief Appends a null-terminated C string.
   * @warning Triggers assertion if resulting size > capacity().
   * @param str C string to append
   * @return Reference to this
   */
  constexpr BasicStaticString& operator+=(const CharT* str) noexcept { return Append(str); }

  /**
   * @brief Accesses character at specified position.
   * @warning Triggers assertion if pos >= size().
   * @param pos Position of the character
   * @return Reference to the character
   */
  [[nodiscard]] constexpr reference operator[](size_type pos) noexcept;

  /**
   * @brief Accesses character at specified position (const).
   * @warning Triggers assertion if pos >= size().
   * @param pos Position of the character
   * @return Const reference to the character
   */
  [[nodiscard]] constexpr const_reference operator[](size_type pos) const noexcept;

  /**
   * @brief Converts to string_view.
   * @return String view of the content
   */
  [[nodiscard]] constexpr operator std::basic_string_view<CharT, Traits>() const noexcept {
    return std::basic_string_view<CharT, Traits>(data_.data(), size_);
  }

  template <size_t OtherCapacity>
  [[nodiscard]] constexpr bool operator==(const BasicStaticString<OtherCapacity, CharT, Traits>& other) const noexcept {
    return View() == other.View();
  }

  [[nodiscard]] constexpr bool operator==(std::basic_string_view<CharT, Traits> other) const noexcept {
    return View() == other;
  }

  [[nodiscard]] constexpr bool operator==(const CharT* other) const noexcept {
    return View() == std::basic_string_view<CharT, Traits>(other);
  }

  template <size_t OtherCapacity>
  [[nodiscard]] constexpr auto operator<=>(const BasicStaticString<OtherCapacity, CharT, Traits>& other) const noexcept
      -> std::strong_ordering;

  [[nodiscard]] constexpr auto operator<=>(std::basic_string_view<CharT, Traits> other) const noexcept
      -> std::strong_ordering;

  [[nodiscard]] constexpr auto operator<=>(const CharT* other) const noexcept -> std::strong_ordering {
    return *this <=> std::basic_string_view<CharT, Traits>(other);
  }

  /**
   * @brief Checks if the string is empty.
   * @return True if Size() == 0
   */
  [[nodiscard]] constexpr bool Empty() const noexcept { return size_ == 0; }

  /**
   * @brief Checks if string starts with prefix.
   * @param sv Prefix to check
   * @return True if string starts with sv
   */
  [[nodiscard]] constexpr bool StartsWith(std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().starts_with(sv);
  }

  /**
   * @brief Checks if string starts with character.
   * @param ch Character to check
   * @return True if string starts with ch
   */
  [[nodiscard]] constexpr bool StartsWith(CharT ch) const noexcept { return !Empty() && Front() == ch; }

  /**
   * @brief Checks if string ends with suffix.
   * @param sv Suffix to check
   * @return True if string ends with sv
   */
  [[nodiscard]] constexpr bool EndsWith(std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().ends_with(sv);
  }

  /**
   * @brief Checks if string ends with character.
   * @param ch Character to check
   * @return True if string ends with ch
   */
  [[nodiscard]] constexpr bool EndsWith(CharT ch) const noexcept { return !Empty() && Back() == ch; }

  /**
   * @brief Checks if string contains substring.
   * @param sv Substring to search for
   * @return True if string contains sv
   */
  [[nodiscard]] constexpr bool Contains(std::basic_string_view<CharT, Traits> sv) const noexcept {
    return View().find(sv) != npos;
  }

  /**
   * @brief Checks if string contains character.
   * @param ch Character to search for
   * @return True if string contains ch
   */
  [[nodiscard]] constexpr bool Contains(CharT ch) const noexcept { return View().find(ch) != npos; }

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
   * @brief Returns the maximum possible number of characters.
   * @return StrCapacity
   */
  [[nodiscard]] static constexpr size_type MaxSize() noexcept { return StrCapacity; }

  /**
   * @brief Returns the capacity.
   * @return StrCapacity
   */
  [[nodiscard]] static constexpr size_type Capacity() noexcept { return StrCapacity; }

  /**
   * @brief Returns the remaining capacity.
   * @return StrCapacity - Size()
   */
  [[nodiscard]] constexpr size_type RemainingCapacity() const noexcept { return StrCapacity - size_; }

  /**
   * @brief Accesses character at specified position with bounds checking.
   * @warning Triggers assertion if pos >= size().
   * @param pos Position of the character
   * @return Reference to the character
   */
  [[nodiscard]] constexpr reference At(size_type pos) noexcept;

  /**
   * @brief Accesses character at specified position with bounds checking (const).
   * @warning Triggers assertion if pos >= size().
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
  [[nodiscard]] constexpr pointer Data() noexcept { return data_.data(); }

  /**
   * @brief Returns pointer to the underlying character array (const).
   * @return Const pointer to the character array
   */
  [[nodiscard]] constexpr const_pointer Data() const noexcept { return data_.data(); }

  /**
   * @brief Returns null-terminated C string.
   * @return Null-terminated character array
   */
  [[nodiscard]] constexpr const_pointer CStr() const noexcept { return data_.data(); }

  /**
   * @brief Returns a string_view of the content.
   * @return String view of the content
   */
  [[nodiscard]] constexpr std::basic_string_view<CharT, Traits> View() const noexcept {
    return std::basic_string_view<CharT, Traits>(data_.data(), size_);
  }

  [[nodiscard]] constexpr iterator begin() noexcept { return data_.data(); }
  [[nodiscard]] constexpr const_iterator begin() const noexcept { return data_.data(); }
  [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return data_.data(); }

  [[nodiscard]] constexpr iterator end() noexcept { return data_.data() + size_; }
  [[nodiscard]] constexpr const_iterator end() const noexcept { return data_.data() + size_; }
  [[nodiscard]] constexpr const_iterator cend() const noexcept { return data_.data() + size_; }

  [[nodiscard]] constexpr reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
  [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
  [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

  [[nodiscard]] constexpr reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
  [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
  [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

private:
  std::array<CharT, StrCapacity + 1> data_{};  ///< Character storage (+ 1 for null terminator)
  size_type size_ = 0;                         ///< Current string length
};

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    std::basic_string_view<CharT, Traits> sv) noexcept {
  HELIOS_ASSERT(sv.size() <= StrCapacity, "String view size exceeds capacity!");
  const size_type copy_len = std::min(sv.size(), StrCapacity);
  Traits::copy(data_.data(), sv.data(), copy_len);
  size_ = copy_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(std::basic_string_view<CharT, Traits> sv,
                                                                           size_type pos, size_type count) noexcept {
  HELIOS_ASSERT(pos <= sv.size(), "Position out of range!");
  const size_type substr_len = std::min(count, sv.size() - pos);
  HELIOS_ASSERT(substr_len <= StrCapacity, "Substring size exceeds capacity!");
  Traits::copy(data_.data(), sv.data() + pos, substr_len);
  size_ = substr_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t N>
  requires(N <= StrCapacity + 1)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(const CharT (&str)[N]) noexcept {
  // N includes null terminator for string literals
  const size_type copy_len = (N > 0 && str[N - 1] == CharT{}) ? N - 1 : N;
  HELIOS_ASSERT(copy_len <= StrCapacity, "String literal size exceeds capacity!");
  Traits::copy(data_.data(), str, copy_len);
  size_ = copy_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(size_type count, CharT ch) noexcept {
  HELIOS_ASSERT(count <= StrCapacity, "Count exceeds capacity!");
  std::ranges::fill_n(data_.begin(), count, ch);
  size_ = count;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(const CharT* str, size_type len) noexcept {
  HELIOS_ASSERT(len <= StrCapacity, "Length exceeds capacity!");
  Traits::copy(data_.data(), str, len);
  size_ = len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::input_iterator InputIt>
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(InputIt first, InputIt last) noexcept {
  size_type count = 0;
  for (auto it = first; it != last && count < StrCapacity; ++it, ++count) {
    data_[count] = *it;
  }
  size_ = count;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(std::from_range_t, Range&& rg) noexcept {
  size_type count = 0;
  for (auto&& ch : rg) {
    if (count >= StrCapacity)
      break;
    data_[count++] = static_cast<CharT>(ch);
  }
  size_ = count;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    std::initializer_list<CharT> ilist) noexcept {
  HELIOS_ASSERT(ilist.size() <= StrCapacity, "Initializer list size exceeds capacity!");
  Traits::copy(data_.data(), ilist.begin(), ilist.size());
  size_ = ilist.size();
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator=(
    std::basic_string_view<CharT, Traits> sv) noexcept -> BasicStaticString& {
  Assign(sv);
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator=(const CharT* str) noexcept
    -> BasicStaticString& {
  Assign(str);
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr void BasicStaticString<StrCapacity, CharT, Traits>::Clear() noexcept {
  size_ = 0;
  data_[0] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Insert(size_type pos,
                                                                     std::basic_string_view<CharT, Traits> sv) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  HELIOS_ASSERT(size_ + sv.size() <= StrCapacity, "Insert would exceed capacity!");

  // Move existing characters to make room
  if (pos < size_) {
    Traits::move(data_.data() + pos + sv.size(), data_.data() + pos, size_ - pos);
  }

  // Copy new characters
  Traits::copy(data_.data() + pos, sv.data(), sv.size());
  size_ += sv.size();
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Insert(size_type pos, size_type count, CharT ch) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  HELIOS_ASSERT(size_ + count <= StrCapacity, "Insert would exceed capacity!");

  if (pos < size_) {
    Traits::move(data_.data() + pos + count, data_.data() + pos, size_ - pos);
  }

  std::ranges::fill_n(data_.begin() + pos, count, ch);
  size_ += count;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::InsertRange(size_type pos, Range&& rg) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");

  // Calculate range size if possible
  size_type range_size = 0;
  if constexpr (std::ranges::sized_range<Range>) {
    range_size = static_cast<size_type>(std::ranges::size(rg));
    HELIOS_ASSERT(size_ + range_size <= StrCapacity, "Insert would exceed capacity!");

    // Move existing characters
    if (pos < size_) {
      Traits::move(data_.data() + pos + range_size, data_.data() + pos, size_ - pos);
    }

    // Copy from range
    auto dest = data_.begin() + pos;
    for (auto&& ch : rg) {
      *dest++ = static_cast<CharT>(ch);
    }
    size_ += range_size;
  } else {
    // For non-sized ranges, insert one by one
    auto insert_pos = pos;
    for (auto&& ch : rg) {
      HELIOS_ASSERT(size_ < StrCapacity, "Insert would exceed capacity!");
      if (insert_pos < size_) {
        Traits::move(data_.data() + insert_pos + 1, data_.data() + insert_pos, size_ - insert_pos);
      }
      data_[insert_pos++] = static_cast<CharT>(ch);
      ++size_;
    }
  }

  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Erase(size_type pos, size_type count) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  const size_type erase_count = std::min(count, size_ - pos);
  const size_type remaining = size_ - pos - erase_count;
  if (remaining > 0) {
    Traits::move(data_.data() + pos, data_.data() + pos + erase_count, remaining);
  }
  size_ -= erase_count;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr void BasicStaticString<StrCapacity, CharT, Traits>::PushBack(CharT ch) noexcept {
  HELIOS_ASSERT(size_ < StrCapacity, "Cannot PushBack: string is at capacity!");
  data_[size_++] = ch;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr void BasicStaticString<StrCapacity, CharT, Traits>::PopBack() noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot PopBack: string is empty!");
  data_[--size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Append(std::basic_string_view<CharT, Traits> sv) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(size_ + sv.size() <= StrCapacity, "Append would exceed capacity!");
  Traits::copy(data_.data() + size_, sv.data(), sv.size());
  size_ += sv.size();
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Append(size_type count, CharT ch) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(size_ + count <= StrCapacity, "Append would exceed capacity!");
  std::ranges::fill_n(data_.begin() + size_, count, ch);
  size_ += count;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::AppendRange(Range&& rg) noexcept -> BasicStaticString& {
  for (auto&& ch : rg) {
    PushBack(static_cast<CharT>(ch));
  }
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Replace(size_type pos, size_type count,
                                                                      std::basic_string_view<CharT, Traits> sv) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  const size_type replace_count = std::min(count, size_ - pos);
  const size_type new_size = size_ - replace_count + sv.size();
  HELIOS_ASSERT(new_size <= StrCapacity, "Replace would exceed capacity!");

  // Move tail
  const size_type tail_pos = pos + replace_count;
  const size_type tail_len = size_ - tail_pos;
  if (tail_len > 0 && sv.size() != replace_count) {
    Traits::move(data_.data() + pos + sv.size(), data_.data() + tail_pos, tail_len);
  }

  // Copy replacement
  Traits::copy(data_.data() + pos, sv.data(), sv.size());
  size_ = new_size;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Replace(size_type pos, size_type count, size_type count2,
                                                                      CharT ch) noexcept -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  const size_type replace_count = std::min(count, size_ - pos);
  const size_type new_size = size_ - replace_count + count2;
  HELIOS_ASSERT(new_size <= StrCapacity, "Replace would exceed capacity!");

  const size_type tail_pos = pos + replace_count;
  const size_type tail_len = size_ - tail_pos;
  if (tail_len > 0 && count2 != replace_count) {
    Traits::move(data_.data() + pos + count2, data_.data() + tail_pos, tail_len);
  }

  std::ranges::fill_n(data_.begin() + pos, count2, ch);
  size_ = new_size;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::ReplaceWithRange(size_type pos, size_type count,
                                                                               Range&& rg) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");

  const size_type replace_count = std::min(count, size_ - pos);

  if constexpr (std::ranges::sized_range<Range>) {
    const size_type range_size = static_cast<size_type>(std::ranges::size(rg));
    const size_type new_size = size_ - replace_count + range_size;
    HELIOS_ASSERT(new_size <= StrCapacity, "Replace would exceed capacity!");

    const size_type tail_pos = pos + replace_count;
    const size_type tail_len = size_ - tail_pos;
    if (tail_len > 0 && range_size != replace_count) {
      Traits::move(data_.data() + pos + range_size, data_.data() + tail_pos, tail_len);
    }

    auto dest = data_.begin() + pos;
    for (auto&& ch : rg) {
      *dest++ = static_cast<CharT>(ch);
    }
    size_ = new_size;
  } else {
    // For non-sized ranges, use temporary storage
    BasicStaticString temp;
    temp.Assign(View().substr(0, pos));
    temp.AppendRange(std::forward<Range>(rg));
    if (pos + replace_count < size_) {
      temp.Append(View().substr(pos + replace_count));
    }
    *this = std::move(temp);
  }

  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Copy(CharT* dest, size_type count,
                                                                   size_type pos) const noexcept -> size_type {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  const size_type copy_count = std::min(count, size_ - pos);
  Traits::copy(dest, data_.data() + pos, copy_count);
  return copy_count;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr void BasicStaticString<StrCapacity, CharT, Traits>::Resize(size_type count, CharT ch) noexcept {
  HELIOS_ASSERT(count <= StrCapacity, "Resize count exceeds capacity!");
  if (count > size_) {
    std::ranges::fill_n(data_.begin() + size_, count - size_, ch);
  }
  size_ = count;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr void BasicStaticString<StrCapacity, CharT, Traits>::Swap(BasicStaticString& other) noexcept {
  std::swap(data_, other.data_);
  std::swap(size_, other.size_);
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Assign(std::basic_string_view<CharT, Traits> sv) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(sv.size() <= StrCapacity, "String view size exceeds capacity!");
  Traits::copy(data_.data(), sv.data(), sv.size());
  size_ = sv.size();
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Assign(size_type count, CharT ch) noexcept
    -> BasicStaticString& {
  HELIOS_ASSERT(count <= StrCapacity, "Count exceeds capacity!");
  std::ranges::fill_n(data_.begin(), count, ch);
  size_ = count;
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <std::ranges::input_range Range>
  requires std::convertible_to<std::ranges::range_value_t<Range>, CharT>
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::AssignRange(Range&& rg) noexcept -> BasicStaticString& {
  Clear();
  return AppendRange(std::forward<Range>(rg));
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator+=(CharT ch) noexcept -> BasicStaticString& {
  PushBack(ch);
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator[](size_type pos) noexcept -> reference {
  HELIOS_ASSERT(pos < size_, "Position out of range!");
  return data_[pos];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator[](size_type pos) const noexcept
    -> const_reference {
  HELIOS_ASSERT(pos < size_, "Position out of range!");
  return data_[pos];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::At(size_type pos) noexcept -> reference {
  HELIOS_ASSERT(pos < size_, "Position out of range!");
  return data_[pos];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::At(size_type pos) const noexcept -> const_reference {
  HELIOS_ASSERT(pos < size_, "Position out of range!");
  return data_[pos];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Front() noexcept -> reference {
  HELIOS_ASSERT(!Empty(), "String is empty!");
  return data_[0];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Front() const noexcept -> const_reference {
  HELIOS_ASSERT(!Empty(), "String is empty!");
  return data_[0];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Back() noexcept -> reference {
  HELIOS_ASSERT(!Empty(), "String is empty!");
  return data_[size_ - 1];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Back() const noexcept -> const_reference {
  HELIOS_ASSERT(!Empty(), "String is empty!");
  return data_[size_ - 1];
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::Substr(size_type pos, size_type count) const noexcept
    -> BasicStaticString {
  HELIOS_ASSERT(pos <= size_, "Position out of range!");
  const size_type substr_len = std::min(count, size_ - pos);
  return BasicStaticString(data_.data() + pos, substr_len);
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator<=>(
    const BasicStaticString<OtherCapacity, CharT, Traits>& other) const noexcept -> std::strong_ordering {
  const auto result = View().compare(other.View());
  if (result < 0) {
    return std::strong_ordering::less;
  }
  if (result > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator<=>(
    std::basic_string_view<CharT, Traits> other) const noexcept -> std::strong_ordering {
  const auto result = View().compare(other);
  if (result < 0) {
    return std::strong_ordering::less;
  }
  if (result > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(const BasicStaticString& other,
                                                                           size_type pos, size_type count) noexcept {
  HELIOS_ASSERT(pos <= other.Size(), "Position out of range!");
  const size_type substr_len = std::min(count, other.Size() - pos);
  HELIOS_ASSERT(substr_len <= StrCapacity, "Substring size exceeds capacity!");
  Traits::copy(data_.data(), other.Data() + pos, substr_len);
  size_ = substr_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(BasicStaticString&& other, size_type pos,
                                                                           size_type count) noexcept {
  HELIOS_ASSERT(pos <= other.Size(), "Position out of range!");
  const size_type substr_len = std::min(count, other.Size() - pos);
  HELIOS_ASSERT(substr_len <= StrCapacity, "Substring size exceeds capacity!");
  Traits::copy(data_.data(), other.Data() + pos, substr_len);
  size_ = substr_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    const BasicStaticString<OtherCapacity, CharT, Traits>& other) noexcept {
  HELIOS_ASSERT(other.Size() <= StrCapacity, "Source string size exceeds capacity!");
  Traits::copy(data_.data(), other.Data(), other.Size());
  size_ = other.Size();
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    BasicStaticString<OtherCapacity, CharT, Traits>&& other) noexcept {
  HELIOS_ASSERT(other.Size() <= StrCapacity, "Source string size exceeds capacity!");
  Traits::copy(data_.data(), other.Data(), other.Size());
  size_ = other.Size();
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    const BasicStaticString<OtherCapacity, CharT, Traits>& other, size_type pos, size_type count) noexcept {
  HELIOS_ASSERT(pos <= other.Size(), "Position out of range!");
  const size_type substr_len = std::min(count, other.Size() - pos);
  HELIOS_ASSERT(substr_len <= StrCapacity, "Substring size exceeds capacity!");
  Traits::copy(data_.data(), other.Data() + pos, substr_len);
  size_ = substr_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr BasicStaticString<StrCapacity, CharT, Traits>::BasicStaticString(
    BasicStaticString<OtherCapacity, CharT, Traits>&& other, size_type pos, size_type count) noexcept {
  HELIOS_ASSERT(pos <= other.Size(), "Position out of range!");
  const size_type substr_len = std::min(count, other.Size() - pos);
  HELIOS_ASSERT(substr_len <= StrCapacity, "Substring size exceeds capacity!");
  Traits::copy(data_.data(), other.Data() + pos, substr_len);
  size_ = substr_len;
  data_[size_] = CharT{};
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator=(
    const BasicStaticString<OtherCapacity, CharT, Traits>& other) noexcept -> BasicStaticString& {
  HELIOS_ASSERT(other.Size() <= StrCapacity, "Source string size exceeds capacity!");
  Traits::copy(data_.data(), other.Data(), other.Size());
  size_ = other.Size();
  data_[size_] = CharT{};
  return *this;
}

template <size_t StrCapacity, typename CharT, typename Traits>
  requires(StrCapacity > 0)
template <size_t OtherCapacity>
  requires(OtherCapacity < StrCapacity)
constexpr auto BasicStaticString<StrCapacity, CharT, Traits>::operator=(
    BasicStaticString<OtherCapacity, CharT, Traits>&& other) noexcept -> BasicStaticString& {
  HELIOS_ASSERT(other.Size() <= StrCapacity, "Source string size exceeds capacity!");
  Traits::copy(data_.data(), other.Data(), other.Size());
  size_ = other.Size();
  data_[size_] = CharT{};
  return *this;
}

/**
 * @brief Fixed-capacity string of char.
 * @tparam N Maximum capacity
 */
template <size_t N>
using StaticString = BasicStaticString<N, char>;

/**
 * @brief Fixed-capacity string of wchar_t.
 * @tparam N Maximum capacity
 */
template <size_t N>
using StaticWString = BasicStaticString<N, wchar_t>;

/**
 * @brief Fixed-capacity string of char8_t.
 * @tparam N Maximum capacity
 */
template <size_t N>
using StaticU8String = BasicStaticString<N, char8_t>;

/**
 * @brief Fixed-capacity string of char16_t.
 * @tparam N Maximum capacity
 */
template <size_t N>
using StaticU16String = BasicStaticString<N, char16_t>;

/**
 * @brief Fixed-capacity string of char32_t.
 * @tparam N Maximum capacity
 */
template <size_t N>
using StaticU32String = BasicStaticString<N, char32_t>;

/**
 * @brief Concatenates two StaticStrings.
 * @tparam N1 StrCapacity of first string
 * @tparam N2 StrCapacity of second string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs First string
 * @param rhs Second string
 * @return Concatenated string with capacity N1 + N2
 */
template <size_t N1, size_t N2, typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator+(const BasicStaticString<N1, CharT, Traits>& lhs,
                                       const BasicStaticString<N2, CharT, Traits>& rhs) noexcept
    -> BasicStaticString<N1 + N2, CharT, Traits> {
  BasicStaticString<N1 + N2, CharT, Traits> result;
  result.Append(lhs.View());
  result.Append(rhs.View());
  return result;
}

/**
 * @brief Concatenates StaticString with string_view.
 * @warning Triggers assertion if combined length > N.
 * @tparam N StrCapacity of the static string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs Static string
 * @param rhs String view
 * @return Concatenated string (same capacity as lhs)
 */
template <size_t N, typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator+(const BasicStaticString<N, CharT, Traits>& lhs,
                                       std::basic_string_view<CharT, Traits> rhs) noexcept
    -> BasicStaticString<N, CharT, Traits> {
  BasicStaticString<N, CharT, Traits> result = lhs;
  result.Append(rhs);
  return result;
}

/**
 * @brief Concatenates string_view with StaticString.
 * @warning Triggers assertion if combined length > N.
 * @tparam N StrCapacity of the static string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs String view
 * @param rhs Static string
 * @return Concatenated string (same capacity as rhs)
 */
template <size_t N, typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator+(std::basic_string_view<CharT, Traits> lhs,
                                       const BasicStaticString<N, CharT, Traits>& rhs) noexcept
    -> BasicStaticString<N, CharT, Traits> {
  BasicStaticString<N, CharT, Traits> result(lhs);
  result.Append(rhs.View());
  return result;
}

/**
 * @brief Concatenates StaticString with character.
 * @warning Triggers assertion if lhs.Size() >= N.
 * @tparam N StrCapacity of the static string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs Static string
 * @param rhs Character
 * @return Concatenated string (same capacity as lhs)
 */
template <size_t N, typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator+(const BasicStaticString<N, CharT, Traits>& lhs, CharT rhs) noexcept
    -> BasicStaticString<N, CharT, Traits> {
  BasicStaticString<N, CharT, Traits> result = lhs;
  result.PushBack(rhs);
  return result;
}

/**
 * @brief Concatenates character with StaticString.
 * @warning Triggers assertion if rhs.Size() >= N.
 * @tparam N StrCapacity of the static string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs Character
 * @param rhs Static string
 * @return Concatenated string (same capacity as rhs)
 */
template <size_t N, typename CharT, typename Traits>
[[nodiscard]] constexpr auto operator+(CharT lhs, const BasicStaticString<N, CharT, Traits>& rhs) noexcept
    -> BasicStaticString<N, CharT, Traits> {
  BasicStaticString<N, CharT, Traits> result(1, lhs);
  result.Append(rhs.View());
  return result;
}

/**
 * @brief Swaps two BasicStaticStrings.
 * @tparam StrCapacity StrCapacity of the strings
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param lhs First string
 * @param rhs Second string
 */
template <size_t StrCapacity, typename CharT, typename Traits>
constexpr void swap(BasicStaticString<StrCapacity, CharT, Traits>& lhs,
                    BasicStaticString<StrCapacity, CharT, Traits>& rhs) noexcept {
  lhs.Swap(rhs);
}

/**
 * @brief Erases all elements equal to value.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param str String to modify
 * @param value Value to erase
 * @return Number of elements erased
 */
template <size_t StrCapacity, typename CharT, typename Traits>
constexpr auto erase(BasicStaticString<StrCapacity, CharT, Traits>& str, CharT value) noexcept ->
    typename BasicStaticString<StrCapacity, CharT, Traits>::size_type {
  const auto original_size = str.Size();
  auto* new_end = std::remove(str.begin(), str.end(), value);
  str.Resize(static_cast<typename BasicStaticString<StrCapacity, CharT, Traits>::size_type>(new_end - str.begin()));
  return original_size - str.Size();
}

/**
 * @brief Erases all elements satisfying predicate.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @tparam Pred Predicate type
 * @param str String to modify
 * @param pred Unary predicate
 * @return Number of elements erased
 */
template <size_t StrCapacity, typename CharT, typename Traits, typename Pred>
constexpr auto erase_if(BasicStaticString<StrCapacity, CharT, Traits>& str, Pred pred) noexcept ->
    typename BasicStaticString<StrCapacity, CharT, Traits>::size_type {
  const auto original_size = str.Size();
  auto* new_end = std::remove_if(str.begin(), str.end(), pred);
  str.Resize(static_cast<typename BasicStaticString<StrCapacity, CharT, Traits>::size_type>(new_end - str.begin()));
  return original_size - str.Size();
}

/**
 * @brief Outputs a BasicStaticString to an output stream.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param os Output stream
 * @param str String to output
 * @return Reference to the output stream
 */
template <size_t StrCapacity, typename CharT, typename Traits>
inline auto operator<<(std::basic_ostream<CharT, Traits>& os, const BasicStaticString<StrCapacity, CharT, Traits>& str)
    -> std::basic_ostream<CharT, Traits>& {
  return os << str.View();
}

/**
 * @brief Inputs a BasicStaticString from an input stream.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 * @param is Input stream
 * @param str String to input
 * @return Reference to the input stream
 */
template <size_t StrCapacity, typename CharT, typename Traits>
inline auto operator>>(std::basic_istream<CharT, Traits>& is, BasicStaticString<StrCapacity, CharT, Traits>& str)
    -> std::basic_istream<CharT, Traits>& {
  std::basic_string<CharT, Traits> temp;
  is >> temp;
  if (temp.size() <= StrCapacity) {
    str.Assign(temp);
  } else {
    is.setstate(std::ios_base::failbit);
  }
  return is;
}

}  // namespace helios::container

namespace std {

/**
 * @brief Hash support for BasicStaticString.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 */
template <size_t StrCapacity, typename CharT, typename Traits>
struct hash<helios::container::BasicStaticString<StrCapacity, CharT, Traits>> {
  [[nodiscard]] constexpr size_t operator()(
      const helios::container::BasicStaticString<StrCapacity, CharT, Traits>& str) const noexcept {
    return hash<basic_string_view<CharT, Traits>>{}(str.View());
  }
};

/**
 * @brief std::format support for BasicStaticString.
 * @details Enables formatting BasicStaticString using std::format by delegating to string_view formatter.
 * @tparam StrCapacity StrCapacity of the string
 * @tparam CharT Character type
 * @tparam Traits Character traits
 */
template <size_t StrCapacity, typename CharT, typename Traits>
struct formatter<helios::container::BasicStaticString<StrCapacity, CharT, Traits>>
    : formatter<basic_string_view<CharT, Traits>> {
  /**
   * @brief Format the BasicStaticString by delegating to string_view formatter.
   */
  auto format(const helios::container::BasicStaticString<StrCapacity, CharT, Traits>& str, format_context& ctx) const {
    return formatter<basic_string_view<CharT, Traits>>::format(str.View(), ctx);
  }
};

}  // namespace std
