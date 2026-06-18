#include <doctest/doctest.h>

#include <helios/ecs/message/consumed_registry.hpp>

#include <array>
#include <memory_resource>
#include <type_traits>
#include <utility>

using namespace helios::ecs;

namespace {

struct PositionMsg {
  static constexpr bool kConsumable = true;

  float x = 0.0F;
  float y = 0.0F;
};

struct VelocityMsg {
  static constexpr bool kConsumable = true;

  float x = 0.0F;
  float y = 0.0F;
};

struct HealthMsg {
  static constexpr bool kConsumable = true;

  int value = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::ConsumedMessagesRegistry") {
  TEST_CASE("ecs::ConsumedMessagesRegistry::ctor") {
    SUBCASE("Default construction produces an empty registry") {
      const ConsumedMessagesRegistry registry;
      CHECK(registry.Empty());
      CHECK_EQ(registry.TotalConsumedCount(), 0);
    }

    SUBCASE("Allocator construction produces an empty registry") {
      std::array<std::byte, 256> buffer{};
      std::pmr::monotonic_buffer_resource resource(buffer.data(),
                                                   buffer.size());
      const PmrConsumedMessagesRegistry registry(
          std::pmr::polymorphic_allocator<std::byte>{&resource});

      CHECK(registry.Empty());
      CHECK_EQ(registry.TotalConsumedCount(), 0);
    }

    SUBCASE("PMR resource construction uses the provided resource") {
      std::array<std::byte, 256> buffer{};
      std::pmr::monotonic_buffer_resource resource(buffer.data(),
                                                   buffer.size());
      PmrConsumedMessagesRegistry registry(&resource);
      registry.MarkConsumed<PositionMsg>(9);

      CHECK(registry.IsConsumed<PositionMsg>(9));
      CHECK_EQ(registry.TotalConsumedCount(), 1);
    }

    SUBCASE("nullptr constructor is deleted") {
      CHECK_FALSE(
          std::is_constructible_v<ConsumedMessagesRegistry<>, std::nullptr_t>);
      CHECK_FALSE(
          std::is_constructible_v<PmrConsumedMessagesRegistry, std::nullptr_t>);
    }

    SUBCASE("Copy construction produces an independent copy") {
      ConsumedMessagesRegistry src;
      src.MarkConsumed<PositionMsg>(0);
      const ConsumedMessagesRegistry copy(src);

      CHECK(copy.IsConsumed<PositionMsg>(0));
      CHECK_EQ(copy.TotalConsumedCount(), 1);
    }

    SUBCASE("Move construction transfers all entries") {
      ConsumedMessagesRegistry src;
      src.MarkConsumed<PositionMsg>(3);
      const ConsumedMessagesRegistry dst(std::move(src));

      CHECK(dst.IsConsumed<PositionMsg>(3));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::operator=") {
    SUBCASE("Copy assignment replicates entries") {
      ConsumedMessagesRegistry src;
      ConsumedMessagesRegistry dst;

      src.MarkConsumed<PositionMsg>(1);
      dst = src;

      CHECK(dst.IsConsumed<PositionMsg>(1));
      CHECK(src.IsConsumed<PositionMsg>(1));
    }

    SUBCASE("Move assignment transfers entries") {
      ConsumedMessagesRegistry src;
      ConsumedMessagesRegistry dst;

      src.MarkConsumed<PositionMsg>(2);
      dst = std::move(src);

      CHECK(dst.IsConsumed<PositionMsg>(2));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::MarkConsumed (typed)") {
    SUBCASE("A marked index is reported as consumed") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK(registry.IsConsumed<PositionMsg>(0));
    }

    SUBCASE("An unmarked index is not reported as consumed") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK_FALSE(registry.IsConsumed<PositionMsg>(1));
    }

    SUBCASE("Multiple distinct indices can be marked for the same type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(2);
      registry.MarkConsumed<PositionMsg>(5);

      CHECK(registry.IsConsumed<PositionMsg>(0));
      CHECK(registry.IsConsumed<PositionMsg>(2));
      CHECK(registry.IsConsumed<PositionMsg>(5));
      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 3);
    }

    SUBCASE("Marking the same index twice is idempotent") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(4);
      registry.MarkConsumed<PositionMsg>(4);

      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 1);
    }

    SUBCASE("Indices for the same type are stored in sorted order") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(5);
      registry.MarkConsumed<PositionMsg>(1);
      registry.MarkConsumed<PositionMsg>(3);

      const auto span = registry.ConsumedIndicesFor<PositionMsg>();
      REQUIRE_EQ(span.size(), 3);
      CHECK_EQ(span[0], 1);
      CHECK_EQ(span[1], 3);
      CHECK_EQ(span[2], 5);
    }

    SUBCASE("Marking indices for different types are stored independently") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);

      CHECK(registry.IsConsumed<PositionMsg>(0));
      CHECK_FALSE(registry.IsConsumed<PositionMsg>(1));
      CHECK(registry.IsConsumed<VelocityMsg>(1));
      CHECK_FALSE(registry.IsConsumed<VelocityMsg>(0));
    }
  }

  TEST_CASE(
      "ecs::ConsumedMessagesRegistry::MarkConsumed (runtime type index)") {
    SUBCASE("Runtime overload marks the index as consumed") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed(MessageTypeIndex::From<PositionMsg>(), 7);
      CHECK(registry.IsConsumed(MessageTypeIndex::From<PositionMsg>(), 7));
    }

    SUBCASE("Runtime overload is idempotent for duplicate index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed(MessageTypeIndex::From<PositionMsg>(), 2);
      registry.MarkConsumed(MessageTypeIndex::From<PositionMsg>(), 2);
      CHECK_EQ(registry.ConsumedCount(MessageTypeIndex::From<PositionMsg>()),
               1);
    }

    SUBCASE("Typed and runtime overloads target the same storage") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(3);
      CHECK(registry.IsConsumed(MessageTypeIndex::From<PositionMsg>(), 3));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::MergeFrom") {
    SUBCASE("Const lvalue source merges without modifying source") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src_mut;
      src_mut.MarkConsumed<PositionMsg>(1);
      src_mut.MarkConsumed<PositionMsg>(2);
      const ConsumedMessagesRegistry<>& src = src_mut;

      dst.MarkConsumed<PositionMsg>(0);
      dst.MergeFrom(src);

      CHECK(dst.IsConsumed<PositionMsg>(0));
      CHECK(dst.IsConsumed<PositionMsg>(1));
      CHECK(dst.IsConsumed<PositionMsg>(2));
      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 3);
      CHECK(src.IsConsumed<PositionMsg>(1));
      CHECK(src.IsConsumed<PositionMsg>(2));
      CHECK_EQ(src.ConsumedCount<PositionMsg>(), 2);
    }

    SUBCASE("Rvalue source merges and is cleared") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      dst.MarkConsumed<PositionMsg>(0);
      src.MarkConsumed<PositionMsg>(1);
      src.MarkConsumed<PositionMsg>(2);
      dst.MergeFrom(std::move(src));

      CHECK(dst.IsConsumed<PositionMsg>(0));
      CHECK(dst.IsConsumed<PositionMsg>(1));
      CHECK(dst.IsConsumed<PositionMsg>(2));
      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 3);
      CHECK(src.Empty());
      CHECK_EQ(src.TotalConsumedCount(), 0);
    }

    SUBCASE("Merges from a registry using a different allocator type") {
      std::array<std::byte, 256> buffer{};
      std::pmr::monotonic_buffer_resource resource(buffer.data(),
                                                   buffer.size());
      ConsumedMessagesRegistry dst;
      PmrConsumedMessagesRegistry src(&resource);

      dst.MarkConsumed<PositionMsg>(0);
      src.MarkConsumed<PositionMsg>(1);
      src.MarkConsumed<PositionMsg>(3);
      dst.MergeFrom(src);

      CHECK(dst.IsConsumed<PositionMsg>(0));
      CHECK(dst.IsConsumed<PositionMsg>(1));
      CHECK(dst.IsConsumed<PositionMsg>(3));
      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 3);
    }

    SUBCASE("Merge result contains no duplicate indices") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      dst.MarkConsumed<PositionMsg>(1);
      dst.MarkConsumed<PositionMsg>(3);
      src.MarkConsumed<PositionMsg>(2);
      src.MarkConsumed<PositionMsg>(3);
      dst.MergeFrom(src);

      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 3);
      const auto span = dst.ConsumedIndicesFor<PositionMsg>();
      REQUIRE_EQ(span.size(), 3);
      CHECK_EQ(span[0], 1);
      CHECK_EQ(span[1], 2);
      CHECK_EQ(span[2], 3);
    }

    SUBCASE("Merge result is in sorted order") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      dst.MarkConsumed<PositionMsg>(4);
      src.MarkConsumed<PositionMsg>(1);
      src.MarkConsumed<PositionMsg>(2);
      dst.MergeFrom(src);

      const auto span = dst.ConsumedIndicesFor<PositionMsg>();
      REQUIRE_EQ(span.size(), 3);
      CHECK_EQ(span[0], 1);
      CHECK_EQ(span[1], 2);
      CHECK_EQ(span[2], 4);
    }

    SUBCASE("Merging into an empty registry populates it correctly") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      src.MarkConsumed<PositionMsg>(5);
      src.MarkConsumed<PositionMsg>(6);
      dst.MergeFrom(src);

      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 2);
      CHECK(dst.IsConsumed<PositionMsg>(5));
      CHECK(dst.IsConsumed<PositionMsg>(6));
    }

    SUBCASE("Merging an empty registry is a no-op") {
      ConsumedMessagesRegistry dst;
      const ConsumedMessagesRegistry empty;

      dst.MarkConsumed<PositionMsg>(0);
      dst.MergeFrom(empty);

      CHECK_EQ(dst.ConsumedCount<PositionMsg>(), 1);
    }

    SUBCASE("Entries for distinct types are merged independently") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      dst.MarkConsumed<PositionMsg>(0);
      src.MarkConsumed<VelocityMsg>(1);
      dst.MergeFrom(src);

      CHECK(dst.IsConsumed<PositionMsg>(0));
      CHECK(dst.IsConsumed<VelocityMsg>(1));
      CHECK_EQ(dst.TotalConsumedCount(), 2);
    }

    SUBCASE("Source registry is not modified by MergeFrom") {
      ConsumedMessagesRegistry dst;
      ConsumedMessagesRegistry src;

      src.MarkConsumed<PositionMsg>(3);
      dst.MergeFrom(src);

      CHECK(src.IsConsumed<PositionMsg>(3));
      CHECK_EQ(src.ConsumedCount<PositionMsg>(), 1);
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::Clear (all)") {
    SUBCASE("All entries are removed after Clear") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);
      registry.Clear();

      CHECK(registry.Empty());
      CHECK_EQ(registry.TotalConsumedCount(), 0);
    }

    SUBCASE("Clear on an already empty registry is a no-op") {
      ConsumedMessagesRegistry registry;
      registry.Clear();
      CHECK(registry.Empty());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::Clear (typed)") {
    SUBCASE("Clears only entries for the specified type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);
      registry.Clear<PositionMsg>();

      CHECK_FALSE(registry.HasConsumed<PositionMsg>());
      CHECK(registry.HasConsumed<VelocityMsg>());
    }

    SUBCASE("Clear typed on unregistered type is a no-op") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.Clear<VelocityMsg>();

      CHECK(registry.HasConsumed<PositionMsg>());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::Clear (runtime type index)") {
    SUBCASE("Runtime Clear removes only the targeted type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);
      registry.Clear(MessageTypeIndex::From<PositionMsg>());

      CHECK_FALSE(registry.HasConsumed<PositionMsg>());
      CHECK(registry.HasConsumed<VelocityMsg>());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::IsConsumed (typed)") {
    SUBCASE("Returns true for a marked index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(10);
      CHECK(registry.IsConsumed<PositionMsg>(10));
    }

    SUBCASE("Returns false for an unmarked index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK_FALSE(registry.IsConsumed<PositionMsg>(99));
    }

    SUBCASE("Returns false for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      CHECK_FALSE(registry.IsConsumed<PositionMsg>(0));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::IsConsumed (runtime type index)") {
    SUBCASE("Returns true for a marked index via runtime type index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(4);
      CHECK(registry.IsConsumed(MessageTypeIndex::From<PositionMsg>(), 4));
    }

    SUBCASE("Returns false for an unmarked index via runtime type index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(4);
      CHECK_FALSE(
          registry.IsConsumed(MessageTypeIndex::From<PositionMsg>(), 5));
    }

    SUBCASE("Returns false for an unregistered type via runtime type index") {
      const ConsumedMessagesRegistry registry;
      CHECK_FALSE(
          registry.IsConsumed(MessageTypeIndex::From<PositionMsg>(), 0));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::ConsumedIndicesFor (typed)") {
    SUBCASE("Returns a sorted span of consumed indices for the type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(3);
      registry.MarkConsumed<PositionMsg>(1);
      registry.MarkConsumed<PositionMsg>(7);

      const auto span = registry.ConsumedIndicesFor<PositionMsg>();
      REQUIRE_EQ(span.size(), 3);
      CHECK_EQ(span[0], 1);
      CHECK_EQ(span[1], 3);
      CHECK_EQ(span[2], 7);
    }

    SUBCASE("Returns empty span for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      const auto span = registry.ConsumedIndicesFor<PositionMsg>();
      CHECK(span.empty());
    }

    SUBCASE("Returns only indices for the queried type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);
      registry.MarkConsumed<VelocityMsg>(2);

      const auto span = registry.ConsumedIndicesFor<PositionMsg>();
      REQUIRE_EQ(span.size(), 1);
      CHECK_EQ(span[0], 0);
    }
  }

  TEST_CASE(
      "ecs::ConsumedMessagesRegistry::ConsumedIndicesFor (runtime type "
      "index)") {
    SUBCASE("Returns the same span as the typed overload") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(2);
      registry.MarkConsumed<PositionMsg>(5);
      const auto typed_span = registry.ConsumedIndicesFor<PositionMsg>();
      const auto runtime_span =
          registry.ConsumedIndicesFor(MessageTypeIndex::From<PositionMsg>());

      REQUIRE_EQ(typed_span.size(), runtime_span.size());
      CHECK_EQ(typed_span[0], runtime_span[0]);
      CHECK_EQ(typed_span[1], runtime_span[1]);
    }

    SUBCASE("Returns empty span for unregistered type") {
      const ConsumedMessagesRegistry registry;
      const auto span =
          registry.ConsumedIndicesFor(MessageTypeIndex::From<PositionMsg>());
      CHECK(span.empty());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::Empty") {
    SUBCASE("Returns true for a freshly default-constructed registry") {
      const ConsumedMessagesRegistry registry;
      CHECK(registry.Empty());
    }

    SUBCASE("Returns false after at least one index is marked") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK_FALSE(registry.Empty());
    }

    SUBCASE("Returns true again after Clear") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.Clear();

      CHECK(registry.Empty());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::HasConsumed (typed)") {
    SUBCASE("Returns false for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      CHECK_FALSE(registry.HasConsumed<PositionMsg>());
    }

    SUBCASE("Returns true after marking at least one index for the type") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK(registry.HasConsumed<PositionMsg>());
    }

    SUBCASE("Returns false after clearing all entries for the type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.Clear<PositionMsg>();

      CHECK_FALSE(registry.HasConsumed<PositionMsg>());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::HasConsumed (runtime type index)") {
    SUBCASE("Returns false for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      CHECK_FALSE(registry.HasConsumed(MessageTypeIndex::From<PositionMsg>()));
    }

    SUBCASE("Returns true after marking at least one index") {
      ConsumedMessagesRegistry registry;
      registry.MarkConsumed<PositionMsg>(0);
      CHECK(registry.HasConsumed(MessageTypeIndex::From<PositionMsg>()));
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::TotalConsumedCount") {
    SUBCASE("Returns zero for an empty registry") {
      const ConsumedMessagesRegistry registry;
      CHECK_EQ(registry.TotalConsumedCount(), 0);
    }

    SUBCASE("Accumulates count across all registered types") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(1);
      registry.MarkConsumed<VelocityMsg>(2);

      CHECK_EQ(registry.TotalConsumedCount(), 3);
    }

    SUBCASE("Does not double-count duplicate indices within the same type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(0);

      CHECK_EQ(registry.TotalConsumedCount(), 1);
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::ConsumedCount (typed)") {
    SUBCASE("Returns zero for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 0);
    }

    SUBCASE("Returns the number of unique marked indices for the type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(1);
      registry.MarkConsumed<PositionMsg>(2);

      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 3);
    }

    SUBCASE("Does not count duplicate markings") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(5);
      registry.MarkConsumed<PositionMsg>(5);

      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 1);
    }

    SUBCASE("Counts only entries for the queried type") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(1);
      registry.MarkConsumed<VelocityMsg>(2);

      CHECK_EQ(registry.ConsumedCount<PositionMsg>(), 2);
      CHECK_EQ(registry.ConsumedCount<VelocityMsg>(), 1);
    }
  }

  TEST_CASE(
      "ecs::ConsumedMessagesRegistry::ConsumedCount (runtime type index)") {
    SUBCASE("Returns zero for an unregistered type") {
      const ConsumedMessagesRegistry registry;
      CHECK_EQ(registry.ConsumedCount(MessageTypeIndex::From<PositionMsg>()),
               0);
    }

    SUBCASE("Returns the same count as the typed overload") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<PositionMsg>(1);

      CHECK_EQ(registry.ConsumedCount(MessageTypeIndex::From<PositionMsg>()),
               registry.ConsumedCount<PositionMsg>());
    }
  }

  TEST_CASE("ecs::ConsumedMessagesRegistry::Data") {
    SUBCASE("Returns a const reference to the underlying map") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      const auto& data = registry.Data();

      CHECK_FALSE(data.empty());
    }

    SUBCASE("Data map contains an entry for each type that has been marked") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(0);
      registry.MarkConsumed<VelocityMsg>(1);
      registry.MarkConsumed<HealthMsg>(2);

      CHECK_EQ(registry.Data().size(), 3);
    }

    SUBCASE("Data map entries hold correctly sorted indices") {
      ConsumedMessagesRegistry registry;

      registry.MarkConsumed<PositionMsg>(4);
      registry.MarkConsumed<PositionMsg>(2);
      const auto& data = registry.Data();

      const auto it = data.find(MessageTypeIndex::From<PositionMsg>());
      REQUIRE_NE(it, data.end());
      REQUIRE_EQ(it->second.size(), 2);
      CHECK_EQ(it->second[0], 2);
      CHECK_EQ(it->second[1], 4);
    }
  }
}
