#include <doctest/doctest.h>

#include <helios/cstring_view.hpp>

#include <format>
#include <functional>
#include <sstream>
#include <string>
#include <string_view>

using namespace helios;

namespace {

constexpr const char* kHello = "hello";
constexpr const char* kWorld = "world";
constexpr const char* kHelloWorld = "hello world";
constexpr const char* kEmpty = "";
constexpr const char* kAbc = "abc";
constexpr const char* kDef = "def";
constexpr const char* kAlpha = "abcdefghijklmnopqrstuvwxyz";
constexpr const char* kRepeated = "abababab";

}  // namespace

TEST_SUITE("helios::CStringView") {
  TEST_CASE("CStringView::npos: sentinel value") {
    SUBCASE("npos equals size_t max") {
      CHECK_EQ(CStringView::npos, static_cast<size_t>(-1));
    }

    SUBCASE("Find returns npos when not found") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find('z'), CStringView::npos);
    }
  }

  TEST_CASE("CStringView::ctor: from std::string") {
    SUBCASE("Constructs from non-empty string") {
      const std::string str = "test";
      const CStringView csv(str);
      CHECK_EQ(csv.Size(), 4);
      CHECK_EQ(csv.View(), "test");
    }

    SUBCASE("Constructs from empty string") {
      const std::string str;
      const CStringView csv(str);
      CHECK_EQ(csv.Size(), 0);
      CHECK(csv.Empty());
    }

    SUBCASE("Constructs from string with embedded nulls") {
      const std::string str("ab\0cd", 5);
      const CStringView csv(str);
      CHECK_EQ(csv.Size(), 5);
      CHECK_EQ(csv[0], 'a');
      CHECK_EQ(csv[1], 'b');
    }
  }

  TEST_CASE("CStringView::ctor: from const char*") {
    SUBCASE("Constructs from non-empty C string") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Size(), 5);
      CHECK_EQ(csv.View(), "hello");
      CHECK_EQ(csv.Data(), kHello);
    }

    SUBCASE("Constructs from empty C string") {
      const CStringView csv(kEmpty);
      CHECK_EQ(csv.Size(), 0);
      CHECK(csv.Empty());
    }

    SUBCASE("Constructs from string literal") {
      const CStringView csv("literal");
      CHECK_EQ(csv.Size(), 7);
      CHECK_EQ(csv.View(), "literal");
    }
  }

  TEST_CASE("CStringView::ctor: copy and move") {
    SUBCASE("Copy construction preserves data and size") {
      const CStringView original(kHello);
      const CStringView copy(original);
      CHECK_EQ(copy.Size(), original.Size());
      CHECK_EQ(copy.View(), original.View());
      CHECK_EQ(copy.Data(), original.Data());
    }

    SUBCASE("Move construction transfers contents") {
      CStringView original(kHelloWorld);
      const size_t size = original.Size();
      const auto* data = original.Data();
      const CStringView moved(std::move(original));
      CHECK_EQ(moved.Size(), size);
      CHECK_EQ(moved.Data(), data);
      CHECK_EQ(moved.View(), kHelloWorld);
    }

    SUBCASE("Copy assignment preserves data and size") {
      const CStringView original(kWorld);
      const CStringView other(kHello);
      CStringView copy = other;
      copy = original;
      CHECK_EQ(copy.Size(), original.Size());
      CHECK_EQ(copy.View(), original.View());
    }

    SUBCASE("Move assignment transfers contents") {
      CStringView original(kHelloWorld);
      const size_t size = original.Size();
      const auto* data = original.Data();
      CStringView moved(kHello);
      moved = std::move(original);
      CHECK_EQ(moved.Size(), size);
      CHECK_EQ(moved.Data(), data);
    }
  }

  TEST_CASE("CStringView::Assign") {
    SUBCASE("Assign from std::string") {
      CStringView csv(kHello);
      const std::string str = "replaced";
      csv.Assign(str);
      CHECK_EQ(csv.Size(), 8);
      CHECK_EQ(csv.View(), "replaced");
    }

    SUBCASE("Assign from const char*") {
      CStringView csv(kHello);
      csv.Assign(kWorld);
      CHECK_EQ(csv.Size(), 5);
      CHECK_EQ(csv.View(), "world");
    }

    SUBCASE("Assign from empty string") {
      CStringView csv(kHello);
      csv.Assign(kEmpty);
      CHECK_EQ(csv.Size(), 0);
      CHECK(csv.Empty());
    }

    SUBCASE("Assign returns reference to self") {
      CStringView csv(kHello);
      const CStringView& ref = csv.Assign(kWorld);
      CHECK_EQ(&ref, &csv);
    }
  }

  TEST_CASE("CStringView::Copy") {
    SUBCASE("Copies entire string to buffer") {
      const CStringView csv(kHello);
      char buf[10] = {};
      const size_t copied = csv.Copy(buf, 10);
      CHECK_EQ(copied, 5);
      CHECK_EQ(std::string_view(buf, copied), "hello");
    }

    SUBCASE("Copies with pos offset") {
      const CStringView csv(kHelloWorld);
      char buf[10] = {};
      const size_t copied = csv.Copy(buf, 10, 6);
      CHECK_EQ(copied, 5);
      CHECK_EQ(std::string_view(buf, copied), "world");
    }

    SUBCASE("Copy count smaller than remaining string") {
      const CStringView csv(kHello);
      char buf[10] = {};
      const size_t copied = csv.Copy(buf, 3);
      CHECK_EQ(copied, 3);
      CHECK_EQ(std::string_view(buf, copied), "hel");
    }

    SUBCASE("Copy count larger than remaining string clamps") {
      const CStringView csv(kHello);
      char buf[10] = {};
      const size_t copied = csv.Copy(buf, 100);
      CHECK_EQ(copied, 5);
      CHECK_EQ(std::string_view(buf, copied), "hello");
    }

    SUBCASE("Copy with pos at last character copies one char") {
      const CStringView csv(kHello);
      char buf[1] = {};
      const size_t copied = csv.Copy(buf, 1, 4);
      CHECK_EQ(copied, 1);
      CHECK_EQ(buf[0], 'o');
    }
  }

  TEST_CASE("CStringView::Swap") {
    SUBCASE("Member swap exchanges contents") {
      CStringView a(kHello);
      CStringView b(kWorld);
      a.Swap(b);
      CHECK_EQ(a.View(), "world");
      CHECK_EQ(b.View(), "hello");
    }

    SUBCASE("Friend swap exchanges contents") {
      CStringView a(kHello);
      CStringView b(kWorld);
      swap(a, b);
      CHECK_EQ(a.View(), "world");
      CHECK_EQ(b.View(), "hello");
    }

    SUBCASE("Swap with empty string") {
      CStringView a(kHello);
      CStringView b(kEmpty);
      swap(a, b);
      CHECK_EQ(a.View(), "");
      CHECK(a.Empty());
      CHECK_EQ(b.View(), "hello");
    }
  }

  TEST_CASE("CStringView::Compare") {
    SUBCASE("Equal strings return 0 when comparing CStringViews") {
      const CStringView a(kHello);
      const CStringView b(kHello);
      CHECK_EQ(a.Compare(b), 0);
    }

    SUBCASE("Less string returns negative") {
      const CStringView a(kAbc);
      const CStringView b(kDef);
      CHECK_LT(a.Compare(b), 0);
    }

    SUBCASE("Greater string returns positive") {
      const CStringView a(kDef);
      const CStringView b(kAbc);
      CHECK_GT(a.Compare(b), 0);
    }

    SUBCASE("Compare with string_view: equal") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Compare(std::string_view("hello")), 0);
    }

    SUBCASE("Compare with string_view: less") {
      const CStringView csv(kAbc);
      CHECK_LT(csv.Compare(std::string_view("def")), 0);
    }

    SUBCASE("Compare with string_view: greater") {
      const CStringView csv(kDef);
      CHECK_GT(csv.Compare(std::string_view("abc")), 0);
    }

    SUBCASE("Empty strings are equal") {
      const CStringView a(kEmpty);
      const CStringView b(kEmpty);
      CHECK_EQ(a.Compare(b), 0);
    }

    SUBCASE("Empty is less than non-empty") {
      const CStringView a(kEmpty);
      const CStringView b(kHello);
      CHECK_LT(a.Compare(b), 0);
    }
  }

  TEST_CASE("CStringView::Find") {
    SUBCASE("Find substring at beginning") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.Find(std::string_view("hello")), 0);
    }

    SUBCASE("Find substring in middle") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.Find(std::string_view("lo w")), 3);
    }

    SUBCASE("Find substring at end") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.Find(std::string_view("world")), 6);
    }

    SUBCASE("Find substring not present") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find(std::string_view("xyz")), CStringView::npos);
    }

    SUBCASE("Find with pos offset") {
      const CStringView csv(kRepeated);
      CHECK_EQ(csv.Find(std::string_view("ab"), 2), 2);
    }

    SUBCASE("Find character first occurrence") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find('l'), 2);
    }

    SUBCASE("Find character not present") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find('z'), CStringView::npos);
    }

    SUBCASE("Find character with pos offset") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find('l', 3), 3);
    }

    SUBCASE("Find empty substring returns pos") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Find(std::string_view("")), 0);
      CHECK_EQ(csv.Find(std::string_view(""), 3), 3);
    }
  }

  TEST_CASE("CStringView::RFind") {
    SUBCASE("RFind substring at end") {
      const CStringView csv(kRepeated);
      CHECK_EQ(csv.RFind(std::string_view("ab")), 6);
    }

    SUBCASE("RFind substring not present") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.RFind(std::string_view("xyz")), CStringView::npos);
    }

    SUBCASE("RFind with pos limiting search") {
      const CStringView csv(kRepeated);
      CHECK_EQ(csv.RFind(std::string_view("ab"), 3), 2);
    }

    SUBCASE("RFind character last occurrence") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.RFind('l'), 3);
    }

    SUBCASE("RFind character not present") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.RFind('z'), CStringView::npos);
    }

    SUBCASE("RFind character with pos") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.RFind('l', 2), 2);
    }
  }

  TEST_CASE("CStringView::FindFirstOf") {
    SUBCASE("Finds first of any character in set") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.FindFirstOf(std::string_view("ow")), 4);
    }

    SUBCASE("Returns npos when no character matches") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.FindFirstOf(std::string_view("xyz")), CStringView::npos);
    }

    SUBCASE("Finds first of with pos offset") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.FindFirstOf(std::string_view("o"), 5), 7);
    }

    SUBCASE("Finds first of at beginning") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.FindFirstOf(std::string_view("h")), 0);
    }
  }

  TEST_CASE("CStringView::FindLastOf") {
    SUBCASE("Finds last of any character in set") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.FindLastOf(std::string_view("ol")), 9);
    }

    SUBCASE("Returns npos when no character matches") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.FindLastOf(std::string_view("xyz")), CStringView::npos);
    }

    SUBCASE("Finds last of with pos limiting search") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.FindLastOf(std::string_view("o"), 5), 4);
    }
  }

  TEST_CASE("CStringView::FindFirstNotOf") {
    SUBCASE("Finds first character not in set") {
      const CStringView csv(kAbc);
      CHECK_EQ(csv.FindFirstNotOf(std::string_view("a")), 1);
    }

    SUBCASE("Returns npos when all characters match set") {
      const CStringView csv("aaa");
      CHECK_EQ(csv.FindFirstNotOf(std::string_view("a")), CStringView::npos);
    }

    SUBCASE("Finds first not of with pos offset") {
      const CStringView csv("aabc");
      CHECK_EQ(csv.FindFirstNotOf(std::string_view("a"), 1), 2);
    }

    SUBCASE("Returns 0 when first char not in set") {
      const CStringView csv(kAbc);
      CHECK_EQ(csv.FindFirstNotOf(std::string_view("bc")), 0);
    }
  }

  TEST_CASE("CStringView::FindLastNotOf") {
    SUBCASE("Finds last character not in set") {
      const CStringView csv("abca");
      CHECK_EQ(csv.FindLastNotOf(std::string_view("a")), 2);
    }

    SUBCASE("Returns npos when all characters match set") {
      const CStringView csv("aaa");
      CHECK_EQ(csv.FindLastNotOf(std::string_view("a")), CStringView::npos);
    }

    SUBCASE("Finds last not of with pos limiting search") {
      const CStringView csv("abca");
      CHECK_EQ(csv.FindLastNotOf(std::string_view("a"), 1), 1);
    }
  }

  TEST_CASE("CStringView::operator[]") {
    SUBCASE("Accesses character at valid position (mutable)") {
      CStringView csv(kHello);
      CHECK_EQ(csv[0], 'h');
      CHECK_EQ(csv[4], 'o');
    }

    SUBCASE("Accesses character at valid position (const)") {
      const CStringView csv(kHello);
      CHECK_EQ(csv[0], 'h');
      CHECK_EQ(csv[2], 'l');
    }

    SUBCASE("Accesses first and last character") {
      const CStringView csv(kHello);
      CHECK_EQ(csv[0], 'h');
      CHECK_EQ(csv[4], 'o');
    }
  }

  TEST_CASE("CStringView::operator=: string and const char*") {
    SUBCASE("Assigns from std::string") {
      CStringView csv(kHello);
      const std::string str = "assigned";
      csv = str;
      CHECK_EQ(csv.View(), "assigned");
    }

    SUBCASE("Assigns from const char*") {
      CStringView csv(kHello);
      csv = kWorld;
      CHECK_EQ(csv.View(), "world");
    }

    SUBCASE("Assigns from empty string") {
      CStringView csv(kHello);
      csv = kEmpty;
      CHECK(csv.Empty());
    }

    SUBCASE("Assignment returns reference to self") {
      CStringView csv(kHello);
      const CStringView& ref = (csv = kWorld);
      CHECK_EQ(&ref, &csv);
    }
  }

  TEST_CASE("CStringView::operator std::string_view") {
    SUBCASE("Implicit conversion to string_view produces correct content") {
      const CStringView csv(kHello);
      std::string_view sv = csv;
      CHECK_EQ(sv, "hello");
      CHECK_EQ(sv.size(), 5);
    }

    SUBCASE("Conversion from empty CStringView") {
      const CStringView csv(kEmpty);
      std::string_view sv = csv;
      CHECK_EQ(sv, "");
      CHECK(sv.empty());
    }

    SUBCASE("Can pass to functions expecting string_view") {
      const CStringView csv(kHelloWorld);
      const auto len = [](std::string_view sv) { return sv.size(); };
      CHECK_EQ(len(csv), 11);
    }
  }

  TEST_CASE("CStringView::operator<=>") {
    SUBCASE("Equal CStringViews return equal") {
      const CStringView a(kHello);
      const CStringView b(kHello);
      CHECK_EQ(a <=> b, std::strong_ordering::equal);
    }

    SUBCASE("Less CStringView returns less") {
      const CStringView a(kAbc);
      const CStringView b(kDef);
      CHECK_EQ(a <=> b, std::strong_ordering::less);
    }

    SUBCASE("Greater CStringView returns greater") {
      const CStringView a(kDef);
      const CStringView b(kAbc);
      CHECK_EQ(a <=> b, std::strong_ordering::greater);
    }

    SUBCASE("Equal to string_view returns equal") {
      const CStringView csv(kHello);
      CHECK_EQ(csv <=> std::string_view("hello"), std::strong_ordering::equal);
    }

    SUBCASE("Less than string_view returns less") {
      const CStringView csv(kAbc);
      CHECK_EQ(csv <=> std::string_view("def"), std::strong_ordering::less);
    }

    SUBCASE("Greater than string_view returns greater") {
      const CStringView csv(kDef);
      CHECK_EQ(csv <=> std::string_view("abc"), std::strong_ordering::greater);
    }

    SUBCASE("Synthesized relational operators work") {
      const CStringView a(kAbc);
      const CStringView b(kDef);
      CHECK(a < b);
      CHECK_FALSE(a > b);
      CHECK_NE(a.Compare(b), 0);
    }
  }

  TEST_CASE("CStringView::Empty") {
    SUBCASE("Returns false for non-empty string") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.Empty());
    }

    SUBCASE("Returns true for empty string") {
      const CStringView csv(kEmpty);
      CHECK(csv.Empty());
    }
  }

  TEST_CASE("CStringView::StartsWith") {
    SUBCASE("Returns true when starts with prefix string_view") {
      const CStringView csv(kHelloWorld);
      CHECK(csv.StartsWith(std::string_view("hello")));
    }

    SUBCASE("Returns false when does not start with prefix") {
      const CStringView csv(kHelloWorld);
      CHECK_FALSE(csv.StartsWith(std::string_view("world")));
    }

    SUBCASE("Returns true for exact match") {
      const CStringView csv(kHello);
      CHECK(csv.StartsWith(std::string_view("hello")));
    }

    SUBCASE("Returns false for longer prefix") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.StartsWith(std::string_view("hello world")));
    }

    SUBCASE("Returns true when starts with character") {
      const CStringView csv(kHello);
      CHECK(csv.StartsWith('h'));
    }

    SUBCASE("Returns false when does not start with character") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.StartsWith('w'));
    }

    SUBCASE("Empty string does not start with char") {
      const CStringView csv(kEmpty);
      CHECK_FALSE(csv.StartsWith('a'));
    }
  }

  TEST_CASE("CStringView::EndsWith") {
    SUBCASE("Returns true when ends with suffix string_view") {
      const CStringView csv(kHelloWorld);
      CHECK(csv.EndsWith(std::string_view("world")));
    }

    SUBCASE("Returns false when does not end with suffix") {
      const CStringView csv(kHelloWorld);
      CHECK_FALSE(csv.EndsWith(std::string_view("hello")));
    }

    SUBCASE("Returns true for exact match") {
      const CStringView csv(kHello);
      CHECK(csv.EndsWith(std::string_view("hello")));
    }

    SUBCASE("Returns false for longer suffix") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.EndsWith(std::string_view("hello world")));
    }

    SUBCASE("Returns true when ends with character") {
      const CStringView csv(kHello);
      CHECK(csv.EndsWith('o'));
    }

    SUBCASE("Returns false when does not end with character") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.EndsWith('h'));
    }

    SUBCASE("Empty string does not end with char") {
      const CStringView csv(kEmpty);
      CHECK_FALSE(csv.EndsWith('a'));
    }
  }

  TEST_CASE("CStringView::Contains") {
    SUBCASE("Returns true when contains substring") {
      const CStringView csv(kHelloWorld);
      CHECK(csv.Contains(std::string_view("lo w")));
    }

    SUBCASE("Returns false when does not contain substring") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.Contains(std::string_view("xyz")));
    }

    SUBCASE("Returns true when contains character") {
      const CStringView csv(kHello);
      CHECK(csv.Contains('e'));
    }

    SUBCASE("Returns false when does not contain character") {
      const CStringView csv(kHello);
      CHECK_FALSE(csv.Contains('z'));
    }

    SUBCASE("Contains empty substring returns true") {
      const CStringView csv(kHello);
      CHECK(csv.Contains(std::string_view("")));
    }
  }

  TEST_CASE("CStringView::Size and Length") {
    SUBCASE("Size returns correct value for non-empty string") {
      const CStringView csv(kHelloWorld);
      CHECK_EQ(csv.Size(), 11);
    }

    SUBCASE("Size returns 0 for empty string") {
      const CStringView csv(kEmpty);
      CHECK_EQ(csv.Size(), 0);
    }

    SUBCASE("Length returns same value as Size") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Length(), csv.Size());
      CHECK_EQ(csv.Length(), 5);
    }
  }

  TEST_CASE("CStringView::At") {
    SUBCASE("At returns correct character at valid pos (mutable)") {
      CStringView csv(kHello);
      CHECK_EQ(csv.At(0), 'h');
      CHECK_EQ(csv.At(4), 'o');
    }

    SUBCASE("At returns correct character at valid pos (const)") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.At(0), 'h');
      CHECK_EQ(csv.At(2), 'l');
    }

    SUBCASE("At returns reference that aliases Data") {
      CStringView csv(kHello);
      CHECK_EQ(&csv.At(0), csv.Data());
    }
  }

  TEST_CASE("CStringView::Front") {
    SUBCASE("Front returns first character (mutable)") {
      CStringView csv(kHello);
      CHECK_EQ(csv.Front(), 'h');
    }

    SUBCASE("Front returns first character (const)") {
      const CStringView csv(kWorld);
      CHECK_EQ(csv.Front(), 'w');
    }

    SUBCASE("Front aliases first element") {
      CStringView csv(kHello);
      CHECK_EQ(&csv.Front(), csv.Data());
    }
  }

  TEST_CASE("CStringView::Back") {
    SUBCASE("Back returns last character (mutable)") {
      CStringView csv(kHello);
      CHECK_EQ(csv.Back(), 'o');
    }

    SUBCASE("Back returns last character (const)") {
      const CStringView csv(kWorld);
      CHECK_EQ(csv.Back(), 'd');
    }

    SUBCASE("Back for single character string") {
      const CStringView csv("x");
      CHECK_EQ(csv.Back(), 'x');
    }
  }

  TEST_CASE("CStringView::Data") {
    SUBCASE("Data returns pointer to underlying array (mutable)") {
      CStringView csv(kHello);
      CHECK_NE(csv.Data(), nullptr);
      CHECK_EQ(csv.Data()[0], 'h');
    }

    SUBCASE("Data returns pointer to underlying array (const)") {
      const CStringView csv(kHello);
      CHECK_NE(csv.Data(), nullptr);
      CHECK_EQ(csv.Data()[0], 'h');
    }

    SUBCASE("Data is null-terminated") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.Data()[csv.Size()], '\0');
    }

    SUBCASE("Data for empty string is non-null and null-terminated") {
      const CStringView csv(kEmpty);
      CHECK_NE(csv.Data(), nullptr);
      CHECK_EQ(csv.Data()[0], '\0');
    }
  }

  TEST_CASE("CStringView::CStr") {
    SUBCASE("CStr returns same pointer as Data") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.CStr(), csv.Data());
    }

    SUBCASE("CStr is null-terminated") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.CStr()[csv.Size()], '\0');
    }
  }

  TEST_CASE("CStringView::View") {
    SUBCASE("View returns string_view of correct size") {
      const CStringView csv(kHello);
      const auto sv = csv.View();
      CHECK_EQ(sv.size(), 5);
      CHECK_EQ(sv, "hello");
    }

    SUBCASE("View for empty string returns empty string_view") {
      const CStringView csv(kEmpty);
      CHECK(csv.View().empty());
    }

    SUBCASE("View data pointer matches CStr") {
      const CStringView csv(kHello);
      CHECK_EQ(csv.View().data(), csv.CStr());
    }
  }

  TEST_CASE("CStringView::Iterators") {
    SUBCASE("begin/end iterate over characters") {
      const CStringView csv(kHello);
      std::string result;
      for (auto it = csv.begin(); it != csv.end(); ++it) {
        result += *it;
      }
      CHECK_EQ(result, "hello");
    }

    SUBCASE("cbegin/cend iterate over characters") {
      const CStringView csv(kWorld);
      std::string result;
      for (auto it = csv.cbegin(); it != csv.cend(); ++it) {
        result += *it;
      }
      CHECK_EQ(result, "world");
    }

    SUBCASE("rbegin/rend reverse iterate") {
      const CStringView csv(kHello);
      std::string result;
      for (auto it = csv.rbegin(); it != csv.rend(); ++it) {
        result += *it;
      }
      CHECK_EQ(result, "olleh");
    }

    SUBCASE("crbegin/crend const reverse iterate") {
      const CStringView csv(kHello);
      std::string result;
      for (auto it = csv.crbegin(); it != csv.crend(); ++it) {
        result += *it;
      }
      CHECK_EQ(result, "olleh");
    }

    SUBCASE("Range-based for works") {
      const CStringView csv(kHello);
      std::string result;
      for (const char ch : csv) {
        result += ch;
      }
      CHECK_EQ(result, "hello");
    }

    SUBCASE("Empty string iterators: begin == end") {
      const CStringView csv(kEmpty);
      CHECK_EQ(csv.begin(), csv.end());
      CHECK_EQ(csv.cbegin(), csv.cend());
    }

    SUBCASE("Empty string reverse iterators: rbegin == rend") {
      const CStringView csv(kEmpty);
      CHECK_EQ(csv.rbegin(), csv.rend());
      CHECK_EQ(csv.crbegin(), csv.crend());
    }
  }

  TEST_CASE("CStringView::operator<<") {
    SUBCASE("Stream insertion outputs string content") {
      const CStringView csv(kHello);
      std::ostringstream os;
      os << csv;
      CHECK_EQ(os.str(), "hello");
    }

    SUBCASE("Stream insertion of empty string") {
      const CStringView csv(kEmpty);
      std::ostringstream os;
      os << csv;
      CHECK(os.str().empty());
    }

    SUBCASE("Multiple insertions concatenate") {
      const CStringView a(kHello);
      const CStringView b(kWorld);
      std::ostringstream os;
      os << a << ' ' << b;
      CHECK_EQ(os.str(), "hello world");
    }
  }

  TEST_CASE("CStringView::std::hash") {
    SUBCASE("Same content produces same hash") {
      const CStringView a(kHello);
      const CStringView b(kHello);
      std::hash<CStringView> hasher;
      CHECK_EQ(hasher(a), hasher(b));
    }

    SUBCASE("Different content produces different hash") {
      const CStringView a(kHello);
      const CStringView b(kWorld);
      std::hash<CStringView> hasher;
      CHECK_NE(hasher(a), hasher(b));
    }

    SUBCASE("Hash matches string_view hash") {
      const CStringView csv(kHello);
      std::hash<CStringView> csv_hasher;
      std::hash<std::string_view> sv_hasher;
      CHECK_EQ(csv_hasher(csv), sv_hasher("hello"));
    }
  }

  TEST_CASE("CStringView::std::format") {
    SUBCASE("Formats string content correctly") {
      const CStringView csv(kHello);
      const std::string result = std::format("{}", csv);
      CHECK_EQ(result, "hello");
    }

    SUBCASE("Formats with width and alignment") {
      const CStringView csv(kAbc);
      const std::string result = std::format("{:>6}", csv);
      CHECK_EQ(result, "   abc");
    }

    SUBCASE("Formats empty string") {
      const CStringView csv(kEmpty);
      const std::string result = std::format("{}", csv);
      CHECK(result.empty());
    }
  }

  TEST_CASE("CStringView::type aliases") {
    SUBCASE("CStringView is char") {
      CHECK(std::is_same_v<CStringView, BasicCStringView<char>>);
    }

    SUBCASE("CWStringView is wchar_t") {
      CHECK(std::is_same_v<WCStringView, BasicCStringView<wchar_t>>);
    }

    SUBCASE("CU8StringView is char8_t") {
      CHECK(std::is_same_v<U8CStringView, BasicCStringView<char8_t>>);
    }

    SUBCASE("CU16StringView is char16_t") {
      CHECK(std::is_same_v<U16CStringView, BasicCStringView<char16_t>>);
    }

    SUBCASE("CU32StringView is char32_t") {
      CHECK(std::is_same_v<U32CStringView, BasicCStringView<char32_t>>);
    }
  }
}
