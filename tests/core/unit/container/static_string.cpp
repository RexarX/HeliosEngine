#include <doctest/doctest.h>

#include <helios/core/container/static_string.hpp>

#include <algorithm>
#include <format>
#include <string>
#include <string_view>

using namespace helios::container;

TEST_SUITE("StaticString") {
  TEST_CASE("Default construction") {
    StaticString<32> str;
    CHECK(str.Empty());
    CHECK_EQ(str.Size(), 0);
    CHECK_EQ(str.Length(), 0);
    CHECK_EQ(str.Capacity(), 32);
    CHECK_EQ(str.MaxSize(), 32);
    CHECK_EQ(str.RemainingCapacity(), 32);
    CHECK_EQ(std::string_view(str.CStr()), "");
  }

  TEST_CASE("Construction from string_view") {
    StaticString<32> str(std::string_view("Hello"));
    CHECK_FALSE(str.Empty());
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "Hello");
  }

  TEST_CASE("Construction from C string") {
    StaticString<32> str("World");
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "World");
  }

  TEST_CASE("Construction from string literal") {
    StaticString<32> str = "Hello, World!";
    CHECK_EQ(str.Size(), 13);
    CHECK_EQ(str.View(), "Hello, World!");
  }

  TEST_CASE("Construction with count and character") {
    StaticString<32> str(5, 'x');
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "xxxxx");
  }

  TEST_CASE("Construction from pointer and length") {
    const char* text = "Hello, World!";
    StaticString<32> str(text, 5);
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "Hello");
  }

  TEST_CASE("Construction from iterator range") {
    std::string source = "Iterator Test";
    StaticString<32> str(source.begin(), source.end());
    CHECK_EQ(str.Size(), source.size());
    CHECK_EQ(str.View(), source);
  }

  TEST_CASE("Copy construction") {
    StaticString<32> original = "Original";
    StaticString<32> copy = original;
    CHECK_EQ(copy.View(), "Original");
    CHECK_EQ(original.View(), "Original");
  }

  TEST_CASE("Move construction") {
    StaticString<32> original = "Move me";
    StaticString<32> moved = std::move(original);
    CHECK_EQ(moved.View(), "Move me");
  }

  TEST_CASE("Copy assignment") {
    StaticString<32> str1 = "First";
    StaticString<32> str2 = "Second";
    str1 = str2;
    CHECK_EQ(str1.View(), "Second");
    CHECK_EQ(str2.View(), "Second");
  }

  TEST_CASE("Move assignment") {
    StaticString<32> str1 = "First";
    StaticString<32> str2 = "Second";
    str1 = std::move(str2);
    CHECK_EQ(str1.View(), "Second");
  }

  TEST_CASE("Assignment from string_view") {
    StaticString<32> str = "Initial";
    str = std::string_view("New Value");
    CHECK_EQ(str.View(), "New Value");
  }

  TEST_CASE("Assignment from C string") {
    StaticString<32> str = "Initial";
    str = "C String";
    CHECK_EQ(str.View(), "C String");
  }

  TEST_CASE("Element access: at()") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.At(0), 'H');
    CHECK_EQ(str.At(4), 'o');

    str.At(0) = 'J';
    CHECK_EQ(str.View(), "Jello");
  }

  TEST_CASE("Element access: operator[]") {
    StaticString<32> str = "World";
    CHECK_EQ(str[0], 'W');
    CHECK_EQ(str[4], 'd');

    str[0] = 'M';
    CHECK_EQ(str.View(), "Morld");
  }

  TEST_CASE("Element access: front() and back()") {
    StaticString<32> str = "Test";
    CHECK_EQ(str.Front(), 'T');
    CHECK_EQ(str.Back(), 't');

    str.Front() = 'B';
    str.Back() = 'd';
    CHECK_EQ(str.View(), "Besd");
  }

  TEST_CASE("Element access: data() and c_str()") {
    StaticString<32> str = "Data";
    CHECK_EQ(std::string_view(str.Data(), str.Size()), "Data");
    CHECK_EQ(std::string(str.CStr()), "Data");

    // Modify via data()
    str.Data()[0] = 'M';
    CHECK_EQ(str.View(), "Mata");
  }

  TEST_CASE("Conversion to string_view") {
    StaticString<32> str = "Convert";
    std::string_view sv = str;
    CHECK_EQ(sv, "Convert");

    CHECK_EQ(str.View(), "Convert");
  }

  TEST_CASE("Iterators: begin() and end()") {
    StaticString<32> str = "Iterate";

    std::string result;
    for (auto it = str.begin(); it != str.end(); ++it) {
      result += *it;
    }
    CHECK_EQ(result, "Iterate");
  }

  TEST_CASE("Iterators: cbegin() and cend()") {
    const StaticString<32> str = "Const";

    std::string result;
    for (auto it = str.cbegin(); it != str.cend(); ++it) {
      result += *it;
    }
    CHECK_EQ(result, "Const");
  }

  TEST_CASE("Iterators: rbegin() and rend()") {
    StaticString<32> str = "Reverse";

    std::string result;
    for (auto it = str.rbegin(); it != str.rend(); ++it) {
      result += *it;
    }
    CHECK_EQ(result, "esreveR");
  }

  TEST_CASE("Iterators: crbegin() and crend()") {
    const StaticString<32> str = "ConstRev";

    std::string result;
    for (auto it = str.crbegin(); it != str.crend(); ++it) {
      result += *it;
    }
    CHECK_EQ(result, "veRtsnoC");
  }

  TEST_CASE("Range-based for loop") {
    StaticString<32> str = "Range";

    std::string result;
    for (char c : str) {
      result += c;
    }
    CHECK_EQ(result, "Range");
  }

  TEST_CASE("Capacity: empty()") {
    StaticString<32> str;
    CHECK(str.Empty());

    str = "Not empty";
    CHECK_FALSE(str.Empty());

    str.Clear();
    CHECK(str.Empty());
  }

  TEST_CASE("Capacity: size() and length()") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.Length(), 5);
  }

  TEST_CASE("Capacity: max_size() and capacity()") {
    StaticString<64> str;
    CHECK_EQ(str.MaxSize(), 64);
    CHECK_EQ(str.Capacity(), 64);
  }

  TEST_CASE("Capacity: remaining_capacity()") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.RemainingCapacity(), 27);
  }

  TEST_CASE("Modifiers: clear()") {
    StaticString<32> str = "To be cleared";
    CHECK_FALSE(str.Empty());

    str.Clear();
    CHECK(str.Empty());
    CHECK_EQ(str.Size(), 0);
    CHECK_EQ(str.CStr()[0], '\0');
  }

  TEST_CASE("Modifiers: push_back()") {
    StaticString<32> str;
    str.PushBack('H');
    str.PushBack('i');
    CHECK_EQ(str.View(), "Hi");
    CHECK_EQ(str.Size(), 2);
  }

  TEST_CASE("Modifiers: pop_back()") {
    StaticString<32> str = "Hello!";
    str.PopBack();
    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Modifiers: append() string_view") {
    StaticString<32> str = "Hello";
    str.Append(std::string_view(", World!"));
    CHECK_EQ(str.View(), "Hello, World!");
  }

  TEST_CASE("Modifiers: append() count and char") {
    StaticString<32> str = "AB";
    str.Append(3, 'C');
    CHECK_EQ(str.View(), "ABCCC");
  }

  TEST_CASE("Modifiers: append() C string") {
    StaticString<32> str = "Start";
    str.Append(" End");
    CHECK_EQ(str.View(), "Start End");
  }

  TEST_CASE("Modifiers: operator+= string_view") {
    StaticString<32> str = "A";
    str += std::string_view("BC");
    CHECK_EQ(str.View(), "ABC");
  }

  TEST_CASE("Modifiers: operator+= char") {
    StaticString<32> str = "AB";
    str += 'C';
    CHECK_EQ(str.View(), "ABC");
  }

  TEST_CASE("Modifiers: operator+= C string") {
    StaticString<32> str = "Hello";
    str += " World";
    CHECK_EQ(str.View(), "Hello World");
  }

  TEST_CASE("Modifiers: assign() string_view") {
    StaticString<32> str = "Old";
    str.Assign(std::string_view("New Value"));
    CHECK_EQ(str.View(), "New Value");
  }

  TEST_CASE("Modifiers: assign() C string") {
    StaticString<32> str = "Old";
    str.Assign("New");
    CHECK_EQ(str.View(), "New");
  }

  TEST_CASE("Modifiers: assign() count and char") {
    StaticString<32> str = "Old";
    str.Assign(4, 'X');
    CHECK_EQ(str.View(), "XXXX");
  }

  TEST_CASE("Modifiers: resize() expand") {
    StaticString<32> str = "Hi";
    str.Resize(5, 'x');
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "Hixxx");
  }

  TEST_CASE("Modifiers: resize() shrink") {
    StaticString<32> str = "Hello World";
    str.Resize(5);
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "Hello");
  }

  TEST_CASE("Modifiers: erase() from position") {
    StaticString<32> str = "Hello World";
    str.Erase(5);
    CHECK_EQ(str.View(), "Hello");
  }

  TEST_CASE("Modifiers: erase() with count") {
    StaticString<32> str = "Hello World";
    str.Erase(5, 1);  // Erase the space
    CHECK_EQ(str.View(), "HelloWorld");
  }

  TEST_CASE("Modifiers: erase() from middle") {
    StaticString<32> str = "ABCDEF";
    str.Erase(2, 2);  // Erase "CD"
    CHECK_EQ(str.View(), "ABEF");
  }

  TEST_CASE("Modifiers: insert()") {
    StaticString<32> str = "HelloWorld";
    str.Insert(5, std::string_view(" "));
    CHECK_EQ(str.View(), "Hello World");
  }

  TEST_CASE("Modifiers: insert() at beginning") {
    StaticString<32> str = "World";
    str.Insert(0, std::string_view("Hello "));
    CHECK_EQ(str.View(), "Hello World");
  }

  TEST_CASE("Modifiers: insert() at end") {
    StaticString<32> str = "Hello";
    str.Insert(5, std::string_view(" World"));
    CHECK_EQ(str.View(), "Hello World");
  }

  TEST_CASE("Modifiers: replace()") {
    StaticString<32> str = "Hello World";
    str.Replace(6, 5, std::string_view("Universe"));
    CHECK_EQ(str.View(), "Hello Universe");
  }

  TEST_CASE("Modifiers: replace() with shorter string") {
    StaticString<32> str = "Hello Universe";
    str.Replace(6, 8, std::string_view("World"));
    CHECK_EQ(str.View(), "Hello World");
  }

  TEST_CASE("Operations: copy()") {
    StaticString<32> str = "Hello, World!";
    char buffer[10] = {};
    auto copied = str.Copy(buffer, 5, 7);
    CHECK_EQ(copied, 5);
    CHECK_EQ(std::string_view(buffer, 5), "World");
  }

  TEST_CASE("Operations: substr()") {
    StaticString<32> str = "Hello, World!";
    auto sub = str.Substr(7, 5);
    CHECK_EQ(sub.View(), "World");
  }

  TEST_CASE("Operations: substr() to end") {
    StaticString<32> str = "Hello, World!";
    auto sub = str.Substr(7);
    CHECK_EQ(sub.View(), "World!");
  }

  TEST_CASE("Operations: compare()") {
    StaticString<32> str = "Hello";

    CHECK_EQ(str.Compare("Hello"), 0);
    CHECK_LT(str.Compare("Jello"), 0);
    CHECK_GT(str.Compare("Gello"), 0);
  }

  TEST_CASE("Operations: compare() with different capacity") {
    StaticString<32> str1 = "Hello";
    StaticString<64> str2 = "Hello";
    StaticString<64> str3 = "World";

    CHECK_EQ(str1.Compare(str2), 0);
    CHECK_LT(str1.Compare(str3), 0);
  }

  TEST_CASE("Search: starts_with() string_view") {
    StaticString<32> str = "Hello, World!";
    CHECK(str.StartsWith(std::string_view("Hello")));
    CHECK_FALSE(str.StartsWith(std::string_view("World")));
  }

  TEST_CASE("Search: starts_with() char") {
    StaticString<32> str = "Hello";
    CHECK(str.StartsWith('H'));
    CHECK_FALSE(str.StartsWith('W'));
  }

  TEST_CASE("Search: ends_with() string_view") {
    StaticString<32> str = "Hello, World!";
    CHECK(str.EndsWith(std::string_view("World!")));
    CHECK_FALSE(str.EndsWith(std::string_view("Hello")));
  }

  TEST_CASE("Search: ends_with() char") {
    StaticString<32> str = "Hello!";
    CHECK(str.EndsWith('!'));
    CHECK_FALSE(str.EndsWith('o'));
  }

  TEST_CASE("Search: contains() string_view") {
    StaticString<32> str = "Hello, World!";
    CHECK(str.Contains(std::string_view("World")));
    CHECK(str.Contains(std::string_view(", ")));
    CHECK_FALSE(str.Contains(std::string_view("foo")));
  }

  TEST_CASE("Search: contains() char") {
    StaticString<32> str = "Hello";
    CHECK(str.Contains('e'));
    CHECK_FALSE(str.Contains('z'));
  }

  TEST_CASE("Search: find() string_view") {
    StaticString<32> str = "Hello, World!";
    CHECK_EQ(str.Find(std::string_view("World")), 7);
    CHECK_EQ(str.Find(std::string_view("foo")), StaticString<32>::npos);
  }

  TEST_CASE("Search: find() char") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.Find('l'), 2);
    CHECK_EQ(str.Find('z'), StaticString<32>::npos);
  }

  TEST_CASE("Search: find() with position") {
    StaticString<32> str = "Hello Hello";
    CHECK_EQ(str.Find('H', 0), 0);
    CHECK_EQ(str.Find('H', 1), 6);
  }

  TEST_CASE("Search: rfind() string_view") {
    StaticString<32> str = "Hello Hello";
    CHECK_EQ(str.RFind(std::string_view("Hello")), 6);
  }

  TEST_CASE("Search: rfind() char") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.RFind('l'), 3);
  }

  TEST_CASE("Search: find_first_of()") {
    StaticString<32> str = "Hello, World!";
    CHECK_EQ(str.FindFirstOf(std::string_view("aeiou")), 1);  // 'e'
  }

  TEST_CASE("Search: find_last_of()") {
    StaticString<32> str = "Hello, World!";
    CHECK_EQ(str.FindLastOf(std::string_view("aeiou")), 8);  // 'o'
  }

  TEST_CASE("Search: find_first_not_of()") {
    StaticString<32> str = "aaabbbccc";
    CHECK_EQ(str.FindFirstNotOf(std::string_view("a")), 3);  // 'b'
  }

  TEST_CASE("Search: find_last_not_of()") {
    StaticString<32> str = "aaabbbccc";
    CHECK_EQ(str.FindLastNotOf(std::string_view("c")), 5);  // 'b'
  }

  TEST_CASE("Comparison: operator== with same capacity") {
    StaticString<32> str1 = "Hello";
    StaticString<32> str2 = "Hello";
    StaticString<32> str3 = "World";

    CHECK(str1 == str2);
    CHECK_FALSE(str1 == str3);
  }

  TEST_CASE("Comparison: operator== with different capacity") {
    StaticString<32> str1 = "Hello";
    StaticString<64> str2 = "Hello";
    StaticString<64> str3 = "World";

    CHECK(str1 == str2);
    CHECK_FALSE(str1 == str3);
  }

  TEST_CASE("Comparison: operator== with string_view") {
    StaticString<32> str = "Hello";
    CHECK(str == std::string_view("Hello"));
    CHECK_FALSE(str == std::string_view("World"));
  }

  TEST_CASE("Comparison: operator== with C string") {
    StaticString<32> str = "Hello";
    CHECK(str == "Hello");
    CHECK_FALSE(str == "World");
  }

  TEST_CASE("Comparison: operator<=>") {
    StaticString<32> str1 = "Apple";
    StaticString<32> str2 = "Banana";
    StaticString<32> str3 = "Apple";

    CHECK(str1 < str2);
    CHECK(str2 > str1);
    CHECK(str1 <= str3);
    CHECK(str1 >= str3);
  }

  TEST_CASE("Comparison: operator<=> with different capacity") {
    StaticString<32> str1 = "Apple";
    StaticString<64> str2 = "Banana";

    CHECK(str1 < str2);
    CHECK(str2 > str1);
  }

  TEST_CASE("Comparison: operator<=> with string_view") {
    StaticString<32> str = "Hello";
    CHECK(str < std::string_view("World"));
    CHECK(str > std::string_view("Aello"));
  }

  TEST_CASE("Non-member: operator+ two StaticStrings") {
    StaticString<16> str1 = "Hello";
    StaticString<16> str2 = " World";
    auto result = str1 + str2;

    CHECK_EQ(result.View(), "Hello World");
    CHECK_EQ(result.Capacity(), 32);
  }

  TEST_CASE("Non-member: operator+ StaticString and string_view") {
    StaticString<32> str = "Hello";
    auto result = str + std::string_view(" World");

    CHECK_EQ(result.View(), "Hello World");
  }

  TEST_CASE("Non-member: operator+ string_view and StaticString") {
    StaticString<32> str = " World";
    auto result = std::string_view("Hello") + str;

    CHECK_EQ(result.View(), "Hello World");
  }

  TEST_CASE("Different character types: wchar_t") {
    StaticWString<32> str = L"Hello";
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), L"Hello");
  }

  TEST_CASE("Different character types: char8_t") {
    StaticU8String<32> str(u8"Hello");
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), u8"Hello");
  }

  TEST_CASE("Different character types: char16_t") {
    StaticU16String<32> str(u"Hello");
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), u"Hello");
  }

  TEST_CASE("Different character types: char32_t") {
    StaticU32String<32> str(U"Hello");
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), U"Hello");
  }

  TEST_CASE("Constexpr construction") {
    constexpr StaticString<32> str = "Constexpr";
    static_assert(str.Size() == 9);
    static_assert(str.Capacity() == 32);
    CHECK_EQ(str.View(), "Constexpr");
  }

  TEST_CASE("Constexpr operations") {
    constexpr auto make_string = []() {
      StaticString<32> str = "Hello";
      return str;
    };

    constexpr auto str = make_string();
    static_assert(str.Size() == 5);
    CHECK_EQ(str.View(), "Hello");
  }

  TEST_CASE("Edge case: Empty string operations") {
    StaticString<32> str;

    CHECK(str.Empty());
    CHECK_EQ(str.Find('a'), StaticString<32>::npos);
    CHECK_FALSE(str.Contains('a'));
    CHECK_FALSE(str.StartsWith('a'));
    CHECK_FALSE(str.EndsWith('a'));

    auto sub = str.Substr(0, 0);
    CHECK(sub.Empty());
  }

  TEST_CASE("Edge case: Full capacity") {
    StaticString<5> str = "Hello";
    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.RemainingCapacity(), 0);
  }

  TEST_CASE("Edge case: Single character") {
    StaticString<32> str(1, 'X');
    CHECK_EQ(str.Size(), 1);
    CHECK_EQ(str.Front(), 'X');
    CHECK_EQ(str.Back(), 'X');
  }

  TEST_CASE("npos value") {
    CHECK_EQ(StaticString<32>::npos, static_cast<size_t>(-1));
  }

  TEST_CASE("Null termination is maintained") {
    StaticString<32> str = "Hello";
    CHECK_EQ(str.Data()[5], '\0');

    str.PushBack('!');
    CHECK_EQ(str.Data()[6], '\0');

    str.PopBack();
    CHECK_EQ(str.Data()[5], '\0');

    str.Clear();
    CHECK_EQ(str.Data()[0], '\0');
  }

  TEST_CASE("STL algorithm compatibility") {
    StaticString<32> str = "dcba";

    std::sort(str.begin(), str.end());
    CHECK_EQ(str.View(), "abcd");

    std::reverse(str.begin(), str.end());
    CHECK_EQ(str.View(), "dcba");

    auto it = std::find(str.begin(), str.end(), 'c');
    CHECK_NE(it, str.end());
    CHECK_EQ(*it, 'c');
  }

  TEST_CASE("Type aliases exist") {
    StaticString<32> s1;
    StaticWString<32> s2;
    StaticU8String<32> s3;
    StaticU16String<32> s4;
    StaticU32String<32> s5;

    CHECK(s1.Empty());
    CHECK(s2.Empty());
    CHECK(s3.Empty());
    CHECK(s4.Empty());
    CHECK(s5.Empty());
  }

  TEST_CASE("Modifiers: AssignRange()") {
    StaticString<32> str;
    std::string source = "Hello";

    str.AssignRange(source);
    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Modifiers: AssignRange() with vector") {
    StaticString<32> str("Initial");
    std::vector<char> chars = {'T', 'e', 's', 't'};

    str.AssignRange(chars);
    CHECK_EQ(str.View(), "Test");
    CHECK_EQ(str.Size(), 4);
  }

  TEST_CASE("Modifiers: AssignRange() clears existing content") {
    StaticString<32> str("Something");
    std::string source = "New";

    str.AssignRange(source);
    CHECK_EQ(str.View(), "New");
    CHECK_EQ(str.Size(), 3);
  }

  TEST_CASE("Modifiers: AppendRange()") {
    StaticString<32> str("Hello");
    std::string suffix = " World";

    str.AppendRange(suffix);
    CHECK_EQ(str.View(), "Hello World");
    CHECK_EQ(str.Size(), 11);
  }

  TEST_CASE("Modifiers: AppendRange() with vector") {
    StaticString<32> str("Test");
    std::vector<char> chars = {'!', '!', '!'};

    str.AppendRange(chars);
    CHECK_EQ(str.View(), "Test!!!");
    CHECK_EQ(str.Size(), 7);
  }

  TEST_CASE("Modifiers: AppendRange() with empty range") {
    StaticString<32> str("Hello");
    std::vector<char> empty;

    str.AppendRange(empty);
    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Modifiers: InsertRange() at beginning") {
    StaticString<32> str("World");
    std::string prefix = "Hello ";

    str.InsertRange(0, prefix);
    CHECK_EQ(str.View(), "Hello World");
    CHECK_EQ(str.Size(), 11);
  }

  TEST_CASE("Modifiers: InsertRange() in middle") {
    StaticString<32> str("Hello World");
    std::string insert = "Beautiful ";

    str.InsertRange(6, insert);
    CHECK_EQ(str.View(), "Hello Beautiful World");
    CHECK_EQ(str.Size(), 21);
  }

  TEST_CASE("Modifiers: InsertRange() at end") {
    StaticString<32> str("Hello");
    std::vector<char> chars = {'!', '!', '!'};

    str.InsertRange(str.Size(), chars);
    CHECK_EQ(str.View(), "Hello!!!");
    CHECK_EQ(str.Size(), 8);
  }

  TEST_CASE("Modifiers: InsertRange() with empty range") {
    StaticString<32> str("Test");
    std::vector<char> empty;

    str.InsertRange(2, empty);
    CHECK_EQ(str.View(), "Test");
    CHECK_EQ(str.Size(), 4);
  }

  TEST_CASE("Modifiers: ReplaceWithRange()") {
    StaticString<32> str("Hello World");
    std::string replacement = "C++";

    str.ReplaceWithRange(6, 5, replacement);
    CHECK_EQ(str.View(), "Hello C++");
    CHECK_EQ(str.Size(), 9);
  }

  TEST_CASE("Modifiers: ReplaceWithRange() with longer range") {
    StaticString<32> str("Hi");
    std::string replacement = "Hello";

    str.ReplaceWithRange(0, 2, replacement);
    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Modifiers: ReplaceWithRange() with shorter range") {
    StaticString<32> str("Hello World");
    std::vector<char> replacement = {'H', 'i'};

    str.ReplaceWithRange(0, 5, replacement);
    CHECK_EQ(str.View(), "Hi World");
    CHECK_EQ(str.Size(), 8);
  }

  TEST_CASE("Modifiers: ReplaceWithRange() in middle") {
    StaticString<32> str("abcdef");
    std::string replacement = "XYZ";

    str.ReplaceWithRange(2, 2, replacement);
    CHECK_EQ(str.View(), "abXYZef");
    CHECK_EQ(str.Size(), 7);
  }

  TEST_CASE("Modifiers: ReplaceWithRange() with empty range") {
    StaticString<32> str("Hello World");
    std::vector<char> empty;

    str.ReplaceWithRange(6, 5, empty);
    CHECK_EQ(str.View(), "Hello ");
    CHECK_EQ(str.Size(), 6);
  }

  TEST_CASE("Range operations: chain multiple operations") {
    StaticString<32> str;
    std::string s1 = "Hello";
    std::string s2 = " ";
    std::string s3 = "World";

    str.AssignRange(s1);
    str.AppendRange(s2);
    str.AppendRange(s3);

    CHECK_EQ(str.View(), "Hello World");
    CHECK_EQ(str.Size(), 11);
  }

  // === Heterogeneous Copy/Move Operations ===

  TEST_CASE("Heterogeneous: copy constructor from smaller capacity") {
    StaticString<16> source("Small");
    StaticString<32> dest(source);

    CHECK_EQ(dest.View(), "Small");
    CHECK_EQ(dest.Size(), 5);
    CHECK_EQ(source.View(), "Small");  // Source unchanged
  }

  TEST_CASE("Heterogeneous: copy constructor preserves content") {
    StaticString<8> source("Test");
    StaticString<16> dest(source);

    CHECK_EQ(dest.View(), "Test");
    CHECK_EQ(dest.Size(), 4);
  }

  TEST_CASE("Heterogeneous: move constructor from smaller capacity") {
    StaticString<16> source("Small");
    StaticString<32> dest(std::move(source));

    CHECK_EQ(dest.View(), "Small");
    CHECK_EQ(dest.Size(), 5);
  }

  TEST_CASE("Heterogeneous: move constructor with larger string") {
    StaticString<20> source("Hello World");
    StaticString<32> dest(std::move(source));

    CHECK_EQ(dest.View(), "Hello World");
    CHECK_EQ(dest.Size(), 11);
  }

  TEST_CASE("Heterogeneous: copy assignment from smaller capacity") {
    StaticString<16> source("Small");
    StaticString<32> dest("Initial");

    dest = source;

    CHECK_EQ(dest.View(), "Small");
    CHECK_EQ(dest.Size(), 5);
    CHECK_EQ(source.View(), "Small");  // Source unchanged
  }

  TEST_CASE("Heterogeneous: copy assignment replaces content") {
    StaticString<8> source("New");
    StaticString<32> dest("VeryLongInitialContent");

    dest = source;

    CHECK_EQ(dest.View(), "New");
    CHECK_EQ(dest.Size(), 3);
  }

  TEST_CASE("Heterogeneous: move assignment from smaller capacity") {
    StaticString<16> source("Small");
    StaticString<32> dest("Initial");

    dest = std::move(source);

    CHECK_EQ(dest.View(), "Small");
    CHECK_EQ(dest.Size(), 5);
  }

  TEST_CASE("Heterogeneous: move assignment with larger string") {
    StaticString<20> source("Hello World");
    StaticString<32> dest("Short");

    dest = std::move(source);

    CHECK_EQ(dest.View(), "Hello World");
    CHECK_EQ(dest.Size(), 11);
  }

  TEST_CASE("Heterogeneous: chain heterogeneous operations") {
    StaticString<16> small("Test");
    StaticString<32> medium(small);
    StaticString<64> large(medium);

    CHECK_EQ(large.View(), "Test");
    CHECK_EQ(large.Size(), 4);
  }

  TEST_CASE("Heterogeneous: copy with empty string") {
    StaticString<16> source;
    StaticString<32> dest("NotEmpty");

    dest = source;

    CHECK(dest.Empty());
    CHECK_EQ(dest.Size(), 0);
  }

  TEST_CASE("Heterogeneous: move with empty string") {
    StaticString<16> source;
    StaticString<32> dest("NotEmpty");

    dest = std::move(source);

    CHECK(dest.Empty());
    CHECK_EQ(dest.Size(), 0);
  }

  TEST_CASE("Heterogeneous: different character access after copy") {
    StaticString<12> source("Hello");
    StaticString<32> dest(source);

    CHECK_EQ(dest[0], 'H');
    CHECK_EQ(dest[4], 'o');
    CHECK_EQ(dest.Front(), 'H');
    CHECK_EQ(dest.Back(), 'o');
  }

  TEST_CASE("Heterogeneous: modification after heterogeneous copy") {
    StaticString<16> source("Base");
    StaticString<32> dest(source);

    dest.Append(" Extended");

    CHECK_EQ(dest.View(), "Base Extended");
    CHECK_EQ(source.View(), "Base");  // Source unaffected
  }

  // === std::format Support ===

  TEST_CASE("Format: basic format with StaticString") {
    StaticString<32> str("Hello");
    auto formatted = std::format("{}", str);

    CHECK_EQ(formatted, "Hello");
  }

  TEST_CASE("Format: StaticString with padding") {
    StaticString<32> str("Hi");
    auto formatted = std::format("{:10}", str);

    CHECK_EQ(formatted, "Hi        ");
  }

  TEST_CASE("Format: StaticString with left alignment") {
    StaticString<32> str("Test");
    auto formatted = std::format("{:<8}", str);

    CHECK_EQ(formatted, "Test    ");
  }

  TEST_CASE("Format: StaticString with right alignment") {
    StaticString<32> str("Test");
    auto formatted = std::format("{:>8}", str);

    CHECK_EQ(formatted, "    Test");
  }

  TEST_CASE("Format: StaticString with center alignment") {
    StaticString<32> str("Test");
    auto formatted = std::format("{:^8}", str);

    CHECK_EQ(formatted, "  Test  ");
  }

  TEST_CASE("Format: empty StaticString") {
    StaticString<32> str;
    auto formatted = std::format("{}", str);

    CHECK_EQ(formatted, "");
  }

  TEST_CASE("Format: StaticString in complex format string") {
    StaticString<16> name("World");
    auto formatted = std::format("Hello, {}!", name);

    CHECK_EQ(formatted, "Hello, World!");
  }

  TEST_CASE("Format: multiple StaticStrings") {
    StaticString<16> first("First");
    StaticString<16> second("Second");
    auto formatted = std::format("{} and {}", first, second);

    CHECK_EQ(formatted, "First and Second");
  }

  TEST_CASE("Format: StaticString with width specifier") {
    StaticString<32> str("ABC");
    auto formatted = std::format("{:5}", str);

    CHECK_EQ(formatted, "ABC  ");
  }

  TEST_CASE("Format: long StaticString with truncation spec") {
    StaticString<32> str("VeryLongString");
    auto formatted = std::format("{:.8}", str);

    CHECK_EQ(formatted, "VeryLong");
  }

  TEST_CASE("Format: StaticString with numeric context") {
    StaticString<32> label("Value");
    int value = 42;
    auto formatted = std::format("{}: {}", label, value);

    CHECK_EQ(formatted, "Value: 42");
  }

  TEST_CASE("Format: heterogeneous StaticString in format") {
    StaticString<16> small("Test");
    StaticString<32> large(small);
    auto formatted = std::format("{}", large);

    CHECK_EQ(formatted, "Test");
  }

  TEST_CASE("Format: StaticString via string_view conversion") {
    StaticString<32> str("Convert");
    std::string_view view = str.View();
    auto formatted = std::format("{}", view);

    CHECK_EQ(formatted, "Convert");
  }

  TEST_CASE("Format: chained format operations") {
    StaticString<16> greeting("Hello");
    StaticString<16> target("World");
    auto first = std::format("{}", greeting);
    auto second = std::format("{}", target);
    auto combined = std::format("{} {}", first, second);

    CHECK_EQ(combined, "Hello World");
  }

  TEST_CASE("Format: StaticString with special characters") {
    StaticString<32> str("Hello\nWorld");
    auto formatted = std::format("{}", str);

    CHECK_EQ(formatted, "Hello\nWorld");
  }

  // === New Constructors: from_range_t ===

  TEST_CASE("Constructor: from_range_t with vector") {
    std::vector<char> chars = {'H', 'e', 'l', 'l', 'o'};
    StaticString<32> str(std::from_range, chars);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: from_range_t with string") {
    std::string source = "Test";
    StaticString<32> str(std::from_range, source);

    CHECK_EQ(str.View(), "Test");
    CHECK_EQ(str.Size(), 4);
  }

  TEST_CASE("Constructor: from_range_t with empty range") {
    std::vector<char> empty;
    StaticString<32> str(std::from_range, empty);

    CHECK(str.Empty());
    CHECK_EQ(str.Size(), 0);
  }

  TEST_CASE("Constructor: from_range_t at capacity limit") {
    std::string source = "12345";
    StaticString<5> str(std::from_range, source);

    CHECK_EQ(str.Size(), 5);
    CHECK_EQ(str.View(), "12345");
  }

  // === New Constructors: initializer_list ===

  TEST_CASE("Constructor: initializer_list basic") {
    StaticString<32> str({'H', 'i', '!'});

    CHECK_EQ(str.View(), "Hi!");
    CHECK_EQ(str.Size(), 3);
  }

  TEST_CASE("Constructor: initializer_list empty") {
    StaticString<32> str({});

    CHECK(str.Empty());
    CHECK_EQ(str.Size(), 0);
  }

  TEST_CASE("Constructor: initializer_list single character") {
    StaticString<32> str({'X'});

    CHECK_EQ(str.View(), "X");
    CHECK_EQ(str.Size(), 1);
  }

  TEST_CASE("Constructor: initializer_list at capacity") {
    StaticString<4> str({'a', 'b', 'c', 'd'});

    CHECK_EQ(str.View(), "abcd");
    CHECK_EQ(str.Size(), 4);
  }

  // === New Constructors: string_view with position and count ===

  TEST_CASE("Constructor: string_view substring from position") {
    std::string_view sv("HelloWorld");
    StaticString<32> str(sv, 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: string_view substring with count") {
    std::string_view sv("HelloWorld");
    StaticString<32> str(sv, 0, 5);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: string_view substring middle") {
    std::string_view sv("HelloWorld");
    StaticString<32> str(sv, 5, 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: string_view substring position zero") {
    std::string_view sv("Test");
    StaticString<32> str(sv, 0);

    CHECK_EQ(str.View(), "Test");
    CHECK_EQ(str.Size(), 4);
  }

  // === New Constructors: substring from same capacity ===

  TEST_CASE("Constructor: substring copy from position") {
    StaticString<32> source("HelloWorld");
    StaticString<32> str(source, 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring copy with count") {
    StaticString<32> source("HelloWorld");
    StaticString<32> str(source, 0, 5);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring move from position") {
    StaticString<32> source("HelloWorld");
    StaticString<32> str(std::move(source), 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring move with count") {
    StaticString<32> source("HelloWorld");
    StaticString<32> str(std::move(source), 0, 5);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  // === New Constructors: substring from smaller capacity ===

  TEST_CASE("Constructor: substring copy from smaller capacity") {
    StaticString<16> source("HelloWorld");
    StaticString<32> str(source, 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring copy from smaller with count") {
    StaticString<16> source("HelloWorld");
    StaticString<32> str(source, 0, 5);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring move from smaller capacity") {
    StaticString<16> source("HelloWorld");
    StaticString<32> str(std::move(source), 5);

    CHECK_EQ(str.View(), "World");
    CHECK_EQ(str.Size(), 5);
  }

  TEST_CASE("Constructor: substring move from smaller with count") {
    StaticString<16> source("HelloWorld");
    StaticString<32> str(std::move(source), 0, 5);

    CHECK_EQ(str.View(), "Hello");
    CHECK_EQ(str.Size(), 5);
  }

  // === nullptr deletion test ===

  TEST_CASE("Constructor: nullptr is deleted") {
    // This should not compile if uncommented:
    // StaticString<32> str(nullptr);
    // Just verify that the type exists and we can create strings normally
    StaticString<32> str("valid");
    CHECK_EQ(str.View(), "valid");
  }

  // === Complex constructor scenarios ===

  TEST_CASE("Constructor: from_range with heterogeneous assignment") {
    std::vector<int> nums = {65, 66, 67};  // ASCII for A, B, C
    StaticString<32> str(std::from_range, nums | std::views::transform([](int n) { return static_cast<char>(n); }));

    CHECK_EQ(str.View(), "ABC");
  }

  TEST_CASE("Constructor: initializer_list copy to larger capacity") {
    StaticString<16> small({'H', 'i'});
    StaticString<32> large(small);

    CHECK_EQ(large.View(), "Hi");
    CHECK_EQ(large.Size(), 2);
  }

  TEST_CASE("Constructor: string_view substring empty") {
    std::string_view sv("Test");
    StaticString<32> str(sv, 4, 0);

    CHECK(str.Empty());
  }

  TEST_CASE("Constructor: substring chain multiple levels") {
    StaticString<64> str64("0123456789");
    StaticString<32> str32(str64, 2, 5);  // "23456"
    StaticString<16> str16(str32, 1, 3);  // "345"

    CHECK_EQ(str16.View(), "345");
    CHECK_EQ(str16.Size(), 3);
  }

  TEST_CASE("Constructor: initializer_list special characters") {
    StaticString<32> str({' ', '\t', '\n', 'X'});

    CHECK_EQ(str.Size(), 4);
    CHECK_EQ(str[0], ' ');
    CHECK_EQ(str[1], '\t');
    CHECK_EQ(str[2], '\n');
    CHECK_EQ(str[3], 'X');
  }
}
