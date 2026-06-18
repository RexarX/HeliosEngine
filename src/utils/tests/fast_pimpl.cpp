#include <doctest/doctest.h>

#include <helios/utils/fast_pimpl.hpp>

#include <cstddef>
#include <string_view>
#include <utility>

using namespace helios::utils;

namespace {

struct DummyStruct;

class PimplTest {
public:
  PimplTest(int num, std::string_view str) noexcept;

  void SetNum(int num) noexcept;
  void SetStr(std::string_view str) noexcept;

  [[nodiscard]] int Num() const noexcept;
  [[nodiscard]] std::string_view Str() const noexcept;

private:
  static constexpr size_t kAlignment = alignof(std::string_view);
  static constexpr size_t kSize = kAlignment + sizeof(std::string_view);

  FastPimpl<DummyStruct, kSize, kAlignment> pimpl_;
};

struct DummyStruct {
  int num = 0;
  std::string_view str;
};

PimplTest::PimplTest(int num, std::string_view str) noexcept
    : pimpl_(num, str) {}

void PimplTest::SetNum(int num) noexcept {
  pimpl_->num = num;
}

void PimplTest::SetStr(std::string_view str) noexcept {
  pimpl_->str = str;
}

int PimplTest::Num() const noexcept {
  return pimpl_->num;
}

std::string_view PimplTest::Str() const noexcept {
  return pimpl_->str;
}

}  // namespace

TEST_SUITE("helios::utils::FastPimpl") {
  TEST_CASE("utils::FastPimpl::ctor: basic usage") {
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
