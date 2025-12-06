#include <doctest/doctest.h>
#include <helios/core/utils/fast_pimpl.hpp>

#include <string>
#include <utility>

namespace {

struct DummyStruct {
  int x = 0;
  std::string s;
};

}  // namespace

TEST_SUITE("utils::FastPimpl") {
  TEST_CASE("FastPimpl::ctor: basic usage") {
    helios::utils::FastPimpl<DummyStruct, sizeof(DummyStruct), alignof(DummyStruct)> pimpl(42, "helios");
    CHECK_EQ(pimpl->x, 42);
    CHECK_EQ(pimpl->s, "helios");

    (*pimpl).x = 100;
    CHECK_EQ(pimpl->x, 100);

    SUBCASE("Copy and move") {
      auto pimpl_copy = pimpl;
      CHECK_EQ(pimpl_copy->x, 100);
      auto pimpl_move = std::move(pimpl_copy);
      CHECK_EQ(pimpl_move->x, 100);
    }
  }

}  // TEST_SUITE
