#include <doctest/doctest.h>

#include <helios/ecs/message/async_wrapper.hpp>

#include <string_view>
#include <utility>

using namespace helios::ecs;

namespace {

struct EventMsg {
  static constexpr bool kAsync = true;
  static constexpr std::string_view kName = "EventMsg";

  int id = 0;

  constexpr bool operator==(const EventMsg&) const noexcept = default;
};

struct CounterMsg {
  static constexpr bool kAsync = true;

  int count = 0;

  constexpr bool operator==(const CounterMsg&) const noexcept = default;
};

}  // namespace

TEST_SUITE("helios::ecs::AsyncMessageWrapper") {
  TEST_CASE("ecs::AsyncMessageWrapper::ctor") {
    SUBCASE("Explicit move constructor stores the message") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{42});
      CHECK_EQ(wrapper->id, 42);
    }

    SUBCASE("Copy constructor produces an independent copy") {
      AsyncMessageWrapper<EventMsg> original(EventMsg{7});
      const AsyncMessageWrapper<EventMsg> copy(
          original);  // NOLINT(performance-unnecessary-copy-initialization)

      CHECK_EQ(copy->id, 7);
      CHECK_EQ(original->id, 7);
    }

    SUBCASE("Move constructor transfers ownership") {
      AsyncMessageWrapper<EventMsg> src(EventMsg{99});
      AsyncMessageWrapper<EventMsg> dst(std::move(src));
      CHECK_EQ(dst->id, 99);
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::operator=") {
    SUBCASE("Copy assignment replicates the message") {
      AsyncMessageWrapper<EventMsg> src(EventMsg{10});
      AsyncMessageWrapper<EventMsg> dst(EventMsg{0});

      dst = src;

      CHECK_EQ(dst->id, 10);
      CHECK_EQ(src->id, 10);
    }

    SUBCASE("Move assignment transfers the message") {
      AsyncMessageWrapper<EventMsg> src(EventMsg{55});
      AsyncMessageWrapper<EventMsg> dst(EventMsg{0});

      dst = std::move(src);

      CHECK_EQ(dst->id, 55);
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::operator->") {
    SUBCASE("Mutable operator-> provides write access to the message") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{1});
      wrapper->id = 100;
      CHECK_EQ(wrapper->id, 100);
    }

    SUBCASE("Const operator-> provides read access to the message") {
      const AsyncMessageWrapper<EventMsg> wrapper(EventMsg{5});
      CHECK_EQ(wrapper->id, 5);
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::operator*") {
    SUBCASE("Mutable dereference returns a reference to the message") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{3});

      EventMsg& ref = *wrapper;
      ref.id = 77;

      CHECK_EQ(wrapper->id, 77);
    }

    SUBCASE("Const dereference returns a const reference to the message") {
      const AsyncMessageWrapper<EventMsg> wrapper(EventMsg{9});
      const EventMsg& ref = *wrapper;
      CHECK_EQ(ref.id, 9);
    }

    SUBCASE("Dereference and operator-> refer to the same object") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{21});
      CHECK_EQ(&(*wrapper), wrapper.operator->());
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::Name") {
    SUBCASE("Returns the custom kName when the message type provides one") {
      const AsyncMessageWrapper<EventMsg> wrapper(EventMsg{});
      CHECK_EQ(wrapper.Name(), "EventMsg");
    }

    SUBCASE("Returns a non-empty fallback name when no kName is provided") {
      const AsyncMessageWrapper<CounterMsg> wrapper(CounterMsg{});
      CHECK_FALSE(wrapper.Name().empty());
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::TypeIndex") {
    SUBCASE("TypeIndex matches the free-function result for the same type") {
      const AsyncMessageWrapper<EventMsg> wrapper(EventMsg{});
      CHECK_EQ(wrapper.TypeIndex(), MessageTypeIndex::From<EventMsg>());
    }

    SUBCASE("TypeIndex differs between distinct message types") {
      const AsyncMessageWrapper<EventMsg> a(EventMsg{});
      const AsyncMessageWrapper<CounterMsg> b(CounterMsg{});
      CHECK_NE(a.TypeIndex(), b.TypeIndex());
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::Unwrap") {
    SUBCASE("Mutable Unwrap returns a reference to the stored message") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{33});

      EventMsg& ref = wrapper.Unwrap();
      ref.id = 44;

      CHECK_EQ(wrapper->id, 44);
    }

    SUBCASE("Const Unwrap returns a const reference to the stored message") {
      const AsyncMessageWrapper<EventMsg> wrapper(EventMsg{66});
      const EventMsg& ref = wrapper.Unwrap();
      CHECK_EQ(ref.id, 66);
    }

    SUBCASE("Unwrap and operator* refer to the same object") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{11});
      CHECK_EQ(&wrapper.Unwrap(), &(*wrapper));
    }
  }

  TEST_CASE("ecs::AsyncMessageWrapper::Take") {
    SUBCASE("Take returns the message with the correct value") {
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{88});
      const EventMsg taken = wrapper.Take();
      CHECK_EQ(taken.id, 88);
    }

    SUBCASE("Returned value from Take equals the original message") {
      const EventMsg original{42};
      AsyncMessageWrapper<EventMsg> wrapper(EventMsg{original});

      const EventMsg taken = wrapper.Take();

      CHECK_EQ(taken, original);
    }
  }
}
