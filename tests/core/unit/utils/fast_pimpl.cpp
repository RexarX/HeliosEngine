#include <doctest/doctest.h>

#include <helios/core/utils/fast_pimpl.hpp>

#include <cstddef>
#include <string_view>
#include <utility>

namespace {

struct DummyStruct;

class PimplTest {
public:
  constexpr PimplTest(int num, std::string_view str) noexcept;

  constexpr void SetNum(int num) noexcept;
  constexpr void SetStr(std::string_view str) noexcept;

  [[nodiscard]] constexpr int Num() const noexcept;
  [[nodiscard]] constexpr std::string_view Str() const noexcept;

private:
  static constexpr size_t kAlignment = alignof(std::string_view);
  static constexpr size_t kSize = kAlignment + sizeof(std::string_view);

  helios::utils::FastPimpl<DummyStruct, kSize, kAlignment> pimpl_;
};

struct DummyStruct {
  int num = 0;
  std::string_view str;
};

constexpr PimplTest::PimplTest(int num, std::string_view str) noexcept : pimpl_(num, str) {}

constexpr void PimplTest::SetNum(int num) noexcept {
  pimpl_->num = num;
}

constexpr void PimplTest::SetStr(std::string_view str) noexcept {
  pimpl_->str = str;
}

constexpr int PimplTest::Num() const noexcept {
  return pimpl_->num;
}

constexpr std::string_view PimplTest::Str() const noexcept {
  return pimpl_->str;
}

}  // namespace

TEST_SUITE("utils::FastPimpl") {
  TEST_CASE("FastPimpl::ctor: basic usage") {
    PimplTest instance(42, "hello");
    CHECK_EQ(instance.Num(), 42);
    CHECK_EQ(instance.Str(), "hello");

    instance.SetNum(100);
    CHECK_EQ(instance.Num(), 100);

    SUBCASE("Copy and move") {
      auto instance_copy = instance;
      CHECK_EQ(instance_copy.Num(), 100);
      auto instance_move = std::move(instance_copy);
      CHECK_EQ(instance_move.Num(), 100);
    }
  }

}  // TEST_SUITE
