#include <doctest/doctest.h>

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/resource/resource.hpp>
#include <helios/ecs/system/access_policy.hpp>

#include <algorithm>

using namespace helios::ecs;

namespace {

// Non-tag components (have data, participate in conflict tracking)
struct Position {
  float x = 0.0F;
  float y = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
};

struct Health {
  int value = 0;
};

struct Transform {
  float m[16] = {};
};

// Tag component (empty, excluded from conflict tracking)
struct Disabled {};

// Plain resources
struct Camera {
  float fov = 90.0F;
};

struct RenderSettings {
  bool vsync = false;
};

struct RenderQueue {
  int count = 0;
};

// Thread-safe resource (should be silently ignored by access policy)
struct ThreadSafeCounter {
  static constexpr bool kThreadSafe = true;

  int value = 0;
};

// Explicitly not thread-safe resource
struct NonThreadSafeData {
  static constexpr bool kThreadSafe = false;

  int value = 0;
};

}  // namespace

TEST_SUITE("helios::ecs::AccessPolicy") {
  TEST_CASE("ecs::AccessPolicy::ctor") {
    SUBCASE("Default ctor produces empty policy") {
      const AccessPolicy policy;
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Copy ctor") {
      const AccessPolicy original =
          AccessPolicyBuilder().Query<const Position&>().Build();

      const AccessPolicy copy = original;

      CHECK(copy.HasComponents());
      CHECK_FALSE(copy.HasResources());
    }

    SUBCASE("Move ctor") {
      AccessPolicy original =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy moved = std::move(original);
      CHECK(moved.HasComponents());
    }
  }

  TEST_CASE("ecs::AccessPolicy::operator=") {
    SUBCASE("Copy assignment") {
      const AccessPolicy original =
          AccessPolicyBuilder().Query<const Position&>().Build();

      AccessPolicy assigned;
      assigned = original;

      CHECK(assigned.HasComponents());
    }

    SUBCASE("Move assignment") {
      AccessPolicy original =
          AccessPolicyBuilder().Query<const Position&>().Build();

      AccessPolicy assigned;
      assigned = std::move(original);

      CHECK(assigned.HasComponents());
    }
  }

  TEST_CASE("ecs::AccessPolicy::Merge") {
    SUBCASE("Merge(const AccessPolicy&) unions components/resources") {
      AccessPolicy dst = AccessPolicyBuilder()
                             .Query<const Position&, Velocity&>()
                             .ReadResources<Camera>()
                             .Build();
      const AccessPolicy src = AccessPolicyBuilder()
                                   .Query<const Health&, Velocity&>()
                                   .WriteResources<RenderQueue>()
                                   .Build();

      dst.Merge(src);

      CHECK(dst.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(dst.HasReadComponent(ComponentTypeIndex::From<Health>()));
      CHECK(dst.HasWriteComponent(ComponentTypeIndex::From<Velocity>()));
      CHECK(dst.HasReadResource(ResourceTypeIndex::From<Camera>()));
      CHECK(dst.HasWriteResource(ResourceTypeIndex::From<RenderQueue>()));
    }

    SUBCASE("Merge(AccessPolicy&&) unions components/resources") {
      AccessPolicy dst = AccessPolicyBuilder().Query<const Position&>().Build();
      AccessPolicy src = AccessPolicyBuilder()
                             .Query<const Health&>()
                             .ReadResources<RenderSettings>()
                             .Build();

      dst.Merge(std::move(src));

      CHECK(dst.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK(dst.HasReadComponent(ComponentTypeIndex::From<Health>()));
      CHECK(dst.HasReadResource(ResourceTypeIndex::From<RenderSettings>()));
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasQueryConflictWith") {
    SUBCASE("Two empty policies do not conflict") {
      const AccessPolicy policy1;
      const AccessPolicy policy2;
      CHECK_FALSE(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE(
        "Policy with no components does not conflict with non-empty policy") {
      const AccessPolicy empty;
      const AccessPolicy with_components =
          AccessPolicyBuilder().Query<const Position&>().Build();

      CHECK_FALSE(empty.HasQueryConflictWith(with_components));
      CHECK_FALSE(with_components.HasQueryConflictWith(empty));
    }

    SUBCASE("Two read-only accesses on the same component do not conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK_FALSE(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE("Write-write conflict on the same component") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&>().Build();

      CHECK(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE("Read-write conflict on the same component is symmetric") {
      const AccessPolicy reader =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy writer =
          AccessPolicyBuilder().Query<Position&>().Build();

      CHECK(reader.HasQueryConflictWith(writer));
      CHECK(writer.HasQueryConflictWith(reader));
    }

    SUBCASE("Accesses on entirely different components do not conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Velocity&>().Build();
      CHECK_FALSE(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE("Tag components are excluded from conflict tracking") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Disabled>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Disabled>().Build();
      CHECK_FALSE(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE(
        "Conflict is detected across components merged from multiple Query "
        "calls") {
      const AccessPolicy policy1 = AccessPolicyBuilder()
                                       .Query<const Position&>()
                                       .Query<Health&>()
                                       .Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Health&>().Build();
      CHECK(policy1.HasQueryConflictWith(policy2));
    }

    SUBCASE(
        "Same component declared in multiple Query calls is deduped and still "
        "conflicts") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Query<Velocity&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&>().Build();
      CHECK(policy1.HasQueryConflictWith(policy2));
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasResourceConflictWith") {
    SUBCASE("Two empty policies do not conflict") {
      const AccessPolicy policy1;
      const AccessPolicy policy2;
      CHECK_FALSE(policy1.HasResourceConflictWith(policy2));
    }

    SUBCASE("Both reading the same resource does not conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK_FALSE(policy1.HasResourceConflictWith(policy2));
    }

    SUBCASE("Read vs write on the same resource conflicts and is symmetric") {
      const AccessPolicy reader =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      const AccessPolicy writer =
          AccessPolicyBuilder().WriteResources<Camera>().Build();

      CHECK(reader.HasResourceConflictWith(writer));
      CHECK(writer.HasResourceConflictWith(reader));
    }

    SUBCASE("Write vs write on the same resource conflicts") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      CHECK(policy1.HasResourceConflictWith(policy2));
    }

    SUBCASE("Accesses to different resources do not conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<RenderQueue>().Build();
      CHECK_FALSE(policy1.HasResourceConflictWith(policy2));
    }

    SUBCASE("Thread-safe resources are excluded and never cause a conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<ThreadSafeCounter>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<ThreadSafeCounter>().Build();

      CHECK_FALSE(policy1.HasResourceConflictWith(policy2));
      CHECK_FALSE(policy1.HasResources());
      CHECK_FALSE(policy2.HasResources());
    }

    SUBCASE(
        "Explicit non-thread-safe resource is tracked and causes conflict") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<NonThreadSafeData>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<NonThreadSafeData>().Build();
      CHECK(policy1.HasResourceConflictWith(policy2));
    }
  }

  TEST_CASE("ecs::AccessPolicy::ConflictsWith") {
    SUBCASE(
        "Returns false when neither component nor resource conflict exists") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK_FALSE(policy1.ConflictsWith(policy2));
    }

    SUBCASE("Returns true when a component conflict exists") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&>().Build();
      CHECK(policy1.ConflictsWith(policy2));
    }

    SUBCASE("Returns true when a resource conflict exists") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      CHECK(policy1.ConflictsWith(policy2));
    }

    SUBCASE("Returns true when both component and resource conflicts exist") {
      const AccessPolicy policy1 = AccessPolicyBuilder()
                                       .Query<Position&>()
                                       .WriteResources<Camera>()
                                       .Build();
      const AccessPolicy policy2 = AccessPolicyBuilder()
                                       .Query<Position&>()
                                       .WriteResources<Camera>()
                                       .Build();
      CHECK(policy1.ConflictsWith(policy2));
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetQueryConflictsWith") {
    SUBCASE("Returns empty vector when no component conflict exists") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK(policy1.GetQueryConflictsWith(policy2).empty());
    }

    SUBCASE("Returns empty vector for two empty policies") {
      const AccessPolicy policy1;
      const AccessPolicy policy2;
      CHECK(policy1.GetQueryConflictsWith(policy2).empty());
    }

    SUBCASE("Write-write: lhs_writes and rhs_writes are both true") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ComponentTypeId::From<Position>());
      CHECK_EQ(conflicts[0].rhs, ComponentTypeId::From<Position>());
      CHECK(conflicts[0].lhs_writes);
      CHECK(conflicts[0].rhs_writes);
    }

    SUBCASE("Write-read: lhs_writes=true, rhs_writes=false") {
      const AccessPolicy writer =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy reader =
          AccessPolicyBuilder().Query<const Position&>().Build();

      const auto conflicts = writer.GetQueryConflictsWith(reader);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ComponentTypeId::From<Position>());
      CHECK_EQ(conflicts[0].rhs, ComponentTypeId::From<Position>());
      CHECK(conflicts[0].lhs_writes);
      CHECK_FALSE(conflicts[0].rhs_writes);
    }

    SUBCASE("Read-write: lhs_writes=false, rhs_writes=true") {
      const AccessPolicy reader =
          AccessPolicyBuilder().Query<const Position&>().Build();
      const AccessPolicy writer =
          AccessPolicyBuilder().Query<Position&>().Build();

      const auto conflicts = reader.GetQueryConflictsWith(writer);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ComponentTypeId::From<Position>());
      CHECK_EQ(conflicts[0].rhs, ComponentTypeId::From<Position>());
      CHECK_FALSE(conflicts[0].lhs_writes);
      CHECK(conflicts[0].rhs_writes);
    }

    SUBCASE("Multiple conflicting components each produce a separate entry") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&, Health&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&, Health&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      CHECK_EQ(conflicts.size(), 2);

      const auto has_position =
          std::ranges::any_of(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Position>();
          });
      const auto has_health =
          std::ranges::any_of(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Health>();
          });
      CHECK(has_position);
      CHECK(has_health);
    }

    SUBCASE(
        "Each conflicting component appears at most once even when matched by "
        "multiple passes") {
      // Position& on both sides matches write-write AND would be re-examined by
      // write-read; the dedup guard must suppress the duplicate.
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&, const Velocity&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      const auto position_count =
          std::ranges::count_if(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Position>();
          });
      CHECK_EQ(position_count, 1);
    }

    SUBCASE("Tag components are absent from conflict results") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Disabled, Position&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Disabled, Position&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      const auto has_disabled =
          std::ranges::any_of(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Disabled>();
          });
      CHECK_FALSE(has_disabled);
    }

    SUBCASE("Non-conflicting components are absent from the result") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&, const Velocity&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Position&, const Health&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      // Velocity is read-only on lhs and absent from rhs — no conflict.
      // Health is absent from lhs — no conflict.
      const auto has_velocity =
          std::ranges::any_of(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Velocity>();
          });
      const auto has_health =
          std::ranges::any_of(conflicts, [](const ComponentConflictInfo& c) {
            return c.lhs == ComponentTypeId::From<Health>();
          });
      CHECK_FALSE(has_velocity);
      CHECK_FALSE(has_health);
    }

    SUBCASE("Components merged from multiple Query calls are still detected") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().Query<Position&>().Query<Health&>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().Query<Health&>().Build();

      const auto conflicts = policy1.GetQueryConflictsWith(policy2);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ComponentTypeId::From<Health>());
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetResourceConflictsWith") {
    SUBCASE("Returns empty vector when no resource conflict exists") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK(policy1.GetResourceConflictsWith(policy2).empty());
    }

    SUBCASE("Returns empty vector for two empty policies") {
      const AccessPolicy policy1;
      const AccessPolicy policy2;
      CHECK(policy1.GetResourceConflictsWith(policy2).empty());
    }

    SUBCASE("Write-write: lhs_writes and rhs_writes are both true") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();

      const auto conflicts = policy1.GetResourceConflictsWith(policy2);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ResourceTypeId::From<Camera>());
      CHECK_EQ(conflicts[0].rhs, ResourceTypeId::From<Camera>());
      CHECK(conflicts[0].lhs_writes);
      CHECK(conflicts[0].rhs_writes);
    }

    SUBCASE("Write-read: lhs_writes=true, rhs_writes=false") {
      const AccessPolicy writer =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy reader =
          AccessPolicyBuilder().ReadResources<Camera>().Build();

      const auto conflicts = writer.GetResourceConflictsWith(reader);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ResourceTypeId::From<Camera>());
      CHECK_EQ(conflicts[0].rhs, ResourceTypeId::From<Camera>());
      CHECK(conflicts[0].lhs_writes);
      CHECK_FALSE(conflicts[0].rhs_writes);
    }

    SUBCASE("Read-write: lhs_writes=false, rhs_writes=true") {
      const AccessPolicy reader =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      const AccessPolicy writer =
          AccessPolicyBuilder().WriteResources<Camera>().Build();

      const auto conflicts = reader.GetResourceConflictsWith(writer);

      REQUIRE_EQ(conflicts.size(), 1);
      CHECK_EQ(conflicts[0].lhs, ResourceTypeId::From<Camera>());
      CHECK_EQ(conflicts[0].rhs, ResourceTypeId::From<Camera>());
      CHECK_FALSE(conflicts[0].lhs_writes);
      CHECK(conflicts[0].rhs_writes);
    }

    SUBCASE("Multiple conflicting resources each produce a separate entry") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera, RenderQueue>().Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder().WriteResources<Camera, RenderQueue>().Build();

      const auto conflicts = policy1.GetResourceConflictsWith(policy2);

      CHECK_EQ(conflicts.size(), 2);

      const auto has_camera = std::ranges::any_of(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<Camera>();
          });
      const auto has_render_queue = std::ranges::any_of(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<RenderQueue>();
          });
      CHECK(has_camera);
      CHECK(has_render_queue);
    }

    SUBCASE(
        "Each conflicting resource appears at most once even when matched by "
        "multiple passes") {
      // Camera written by both sides matches write-write AND would be
      // re-examined by write-read; the dedup guard must suppress the duplicate.
      const AccessPolicy policy1 =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      const AccessPolicy policy2 = AccessPolicyBuilder()
                                       .WriteResources<Camera>()
                                       .ReadResources<RenderSettings>()
                                       .Build();

      const auto conflicts = policy1.GetResourceConflictsWith(policy2);

      const auto camera_count = std::ranges::count_if(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<Camera>();
          });
      CHECK_EQ(camera_count, 1);
    }

    SUBCASE("Thread-safe resources are absent from conflict results") {
      const AccessPolicy policy1 =
          AccessPolicyBuilder()
              .WriteResources<ThreadSafeCounter, Camera>()
              .Build();
      const AccessPolicy policy2 =
          AccessPolicyBuilder()
              .WriteResources<ThreadSafeCounter, Camera>()
              .Build();

      const auto conflicts = policy1.GetResourceConflictsWith(policy2);

      const auto has_counter = std::ranges::any_of(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<ThreadSafeCounter>();
          });
      CHECK_FALSE(has_counter);
    }

    SUBCASE("Non-conflicting resources are absent from the result") {
      const AccessPolicy policy1 = AccessPolicyBuilder()
                                       .WriteResources<Camera>()
                                       .ReadResources<RenderSettings>()
                                       .Build();
      const AccessPolicy policy2 = AccessPolicyBuilder()
                                       .WriteResources<Camera>()
                                       .ReadResources<RenderQueue>()
                                       .Build();

      const auto conflicts = policy1.GetResourceConflictsWith(policy2);

      // RenderSettings (read-only in policy1, absent in policy2) and
      // RenderQueue (absent in policy1) must not appear.
      const auto has_settings = std::ranges::any_of(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<RenderSettings>();
          });
      const auto has_queue = std::ranges::any_of(
          conflicts, [](const ResourceConflictInfo& conflict) {
            return conflict.lhs == ResourceTypeId::From<RenderQueue>();
          });
      CHECK_FALSE(has_settings);
      CHECK_FALSE(has_queue);
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasComponents") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(policy.HasComponents());
    }

    SUBCASE("Returns true after a read component is declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK(policy.HasComponents());
    }

    SUBCASE("Returns true after a write component is declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&>().Build();
      CHECK(policy.HasComponents());
    }

    SUBCASE("Returns false when only resources are declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK_FALSE(policy.HasComponents());
    }

    SUBCASE("Tag-only query produces no tracked components, so returns false") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Disabled>().Build();
      CHECK_FALSE(policy.HasComponents());
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasResources") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Returns true after ReadResources is declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK(policy.HasResources());
    }

    SUBCASE("Returns true after WriteResources is declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<RenderQueue>().Build();
      CHECK(policy.HasResources());
    }

    SUBCASE("Returns false when only a thread-safe resource is declared") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<ThreadSafeCounter>().Build();
      CHECK_FALSE(policy.HasResources());
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasReadComponent") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(
          policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns true for component declared with const reference") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns false for component declared as write") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&>().Build();
      CHECK_FALSE(
          policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns true when component was merged from a later Query call") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Velocity&>()
                                      .Query<const Position&>()
                                      .Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE(
        "Duplicate read declarations across Query calls produce a single "
        "entry") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&>()
                                      .Query<const Velocity&>()
                                      .Build();
      CHECK(policy.HasReadComponent(ComponentTypeIndex::From<Position>()));
      CHECK_EQ(policy.GetReadComponents().size(), 2);
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasWriteComponent") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(
          policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns true for component declared with mutable reference") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&>().Build();
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns false for component declared as read-only") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<const Position&>().Build();
      CHECK_FALSE(
          policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
    }

    SUBCASE("Returns true when component was merged from a later Query call") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Velocity&>()
                                      .Query<Position&>()
                                      .Build();
      CHECK(policy.HasWriteComponent(ComponentTypeIndex::From<Position>()));
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasReadResource") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Returns true for resource declared via ReadResources") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Returns false for resource declared via WriteResources only") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      CHECK_FALSE(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Returns false for a resource not declared at all") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK_FALSE(
          policy.HasReadResource(ResourceTypeIndex::From<RenderQueue>()));
    }
  }

  TEST_CASE("ecs::AccessPolicy::HasWriteResource") {
    SUBCASE("Returns false for default-constructed policy") {
      const AccessPolicy policy;
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Returns true for resource declared via WriteResources") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<Camera>().Build();
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Returns false for resource declared via ReadResources only") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();
      CHECK_FALSE(policy.HasWriteResource(ResourceTypeIndex::From<Camera>()));
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetReadComponents") {
    SUBCASE("Empty policy returns empty span") {
      const AccessPolicy policy;
      CHECK(policy.GetReadComponents().empty());
    }

    SUBCASE("Returns all components declared as read across all Query calls") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&, const Health&>()
                                      .Query<const Velocity&>()
                                      .Build();
      CHECK_EQ(policy.GetReadComponents().size(), 3);
    }

    SUBCASE("Components are kept sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .Query<const Health&, const Position&, const Velocity&>()
              .Build();
      const auto span = policy.GetReadComponents();
      CHECK(std::ranges::is_sorted(span));
    }

    SUBCASE("Duplicate read declarations across Query calls are deduped") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&>()
                                      .Query<const Position&, const Velocity&>()
                                      .Build();
      CHECK_EQ(policy.GetReadComponents().size(), 2);
    }

    SUBCASE("Tag components are not present in the read set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Disabled, const Position&>().Build();

      const auto span = policy.GetReadComponents();

      const auto has_disabled =
          std::ranges::any_of(span, [](const ComponentTypeId& id) {
            return id == ComponentTypeId::From<Disabled>();
          });
      CHECK_FALSE(has_disabled);
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetWriteComponents") {
    SUBCASE("Empty policy returns empty span") {
      const AccessPolicy policy;
      CHECK(policy.GetWriteComponents().empty());
    }

    SUBCASE("Returns all components declared as write across all Query calls") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<Position&, Health&>()
                                      .Query<Velocity&>()
                                      .Build();
      CHECK_EQ(policy.GetWriteComponents().size(), 3);
    }

    SUBCASE("Components are kept sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Health&, Position&, Velocity&>().Build();
      const auto span = policy.GetWriteComponents();
      CHECK(std::ranges::is_sorted(span));
    }

    SUBCASE("Duplicate write declarations across Query calls are deduped") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<Position&>()
                                      .Query<Position&, Velocity&>()
                                      .Build();
      CHECK_EQ(policy.GetWriteComponents().size(), 2);
    }

    SUBCASE("Value copy access (T) is treated as read, not write") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position>().Build();
      CHECK(policy.GetWriteComponents().empty());
      CHECK_EQ(policy.GetReadComponents().size(), 1);
    }

    SUBCASE("Rvalue reference access (T&&) is treated as write") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&&>().Build();
      CHECK(policy.GetReadComponents().empty());
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetReadResources") {
    SUBCASE("Empty policy returns empty span") {
      const AccessPolicy policy;
      CHECK(policy.GetReadResources().empty());
    }

    SUBCASE("Returns all declared read resources") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera, RenderSettings>().Build();
      CHECK_EQ(policy.GetReadResources().size(), 2);
    }

    SUBCASE("Resources are kept sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .ReadResources<RenderQueue, Camera, RenderSettings>()
              .Build();
      const auto span = policy.GetReadResources();
      CHECK(std::ranges::is_sorted(span));
    }

    SUBCASE("Thread-safe resources are not included") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .ReadResources<ThreadSafeCounter, Camera>()
              .Build();
      CHECK_EQ(policy.GetReadResources().size(), 1);
    }
  }

  TEST_CASE("ecs::AccessPolicy::GetWriteResources") {
    SUBCASE("Empty policy returns empty span") {
      const AccessPolicy policy;
      CHECK(policy.GetWriteResources().empty());
    }

    SUBCASE("Returns all declared write resources") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<RenderQueue, Camera>().Build();
      CHECK_EQ(policy.GetWriteResources().size(), 2);
    }

    SUBCASE("Resources are kept sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .WriteResources<RenderQueue, Camera, RenderSettings>()
              .Build();
      const auto span = policy.GetWriteResources();
      CHECK(std::ranges::is_sorted(span));
    }

    SUBCASE("Thread-safe resources are not included") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .WriteResources<ThreadSafeCounter, RenderQueue>()
              .Build();
      CHECK_EQ(policy.GetWriteResources().size(), 1);
    }
  }
}

TEST_SUITE("helios::ecs::AccessPolicyBuilder") {
  TEST_CASE("ecs::AccessPolicyBuilder::ctor") {
    SUBCASE("Default ctor produces builder that builds an empty policy") {
      const AccessPolicy policy = AccessPolicyBuilder().Build();
      CHECK_FALSE(policy.HasComponents());
      CHECK_FALSE(policy.HasResources());
    }

    SUBCASE("Copy ctor preserves accumulated state") {
      AccessPolicyBuilder original;
      original.Query<const Position&>();

      AccessPolicyBuilder copy = original;
      const AccessPolicy policy = copy.Build();

      CHECK(policy.HasComponents());
    }

    SUBCASE("Move ctor preserves accumulated state") {
      AccessPolicyBuilder original;
      original.Query<const Position&>();

      AccessPolicyBuilder moved = std::move(original);
      const AccessPolicy policy = moved.Build();

      CHECK(policy.HasComponents());
    }
  }

  TEST_CASE("ecs::AccessPolicyBuilder::operator=") {
    SUBCASE("Copy assignment preserves accumulated state") {
      AccessPolicyBuilder original;
      original.Query<const Position&>();

      AccessPolicyBuilder copy;
      copy = original;

      CHECK(copy.Build().HasComponents());
    }

    SUBCASE("Move assignment preserves accumulated state") {
      AccessPolicyBuilder original;
      original.Query<const Position&>();

      AccessPolicyBuilder moved;
      moved = std::move(original);

      CHECK(moved.Build().HasComponents());
    }
  }

  TEST_CASE("ecs::AccessPolicyBuilder::Build") {
    SUBCASE(
        "Build produces a policy with the declared components and resources") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&, Velocity&>()
                                      .ReadResources<Camera>()
                                      .WriteResources<RenderQueue>()
                                      .Build();
      CHECK(policy.HasComponents());
      CHECK(policy.HasResources());
    }

    SUBCASE(
        "Builder is chainable and accumulates all declarations into flat "
        "sets") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&>()
                                      .Query<Velocity&>()
                                      .ReadResources<Camera>()
                                      .WriteResources<RenderQueue>()
                                      .Build();

      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
      CHECK_EQ(policy.GetReadResources().size(), 1);
      CHECK_EQ(policy.GetWriteResources().size(), 1);
    }
  }

  TEST_CASE("ecs::AccessPolicyBuilder::Query") {
    SUBCASE("Single read component is placed in the read set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<const Position&>().Build();

      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK(policy.GetWriteComponents().empty());
      CHECK_EQ(policy.GetReadComponents()[0],
               ComponentTypeId::From<Position>());
    }

    SUBCASE("Single write component is placed in the write set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&>().Build();

      CHECK(policy.GetReadComponents().empty());
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
      CHECK_EQ(policy.GetWriteComponents()[0],
               ComponentTypeId::From<Position>());
    }

    SUBCASE(
        "Mixed access in a single query is split into read and write sets") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .Query<const Position&, Velocity&, const Health&>()
              .Build();
      CHECK_EQ(policy.GetReadComponents().size(), 2);
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
    }

    SUBCASE("Tag components are excluded from both read and write sets") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Disabled, const Position&>().Build();
      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK(policy.GetWriteComponents().empty());
    }

    SUBCASE(
        "Multiple Query calls are merged into the flat read and write sets") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&>()
                                      .Query<Velocity&>()
                                      .Query<const Health&>()
                                      .Build();
      CHECK_EQ(policy.GetReadComponents().size(), 2);
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
    }

    SUBCASE("Flat component sets are kept sorted across all Query calls") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Health&, const Position&>()
                                      .Query<Velocity&, Transform&>()
                                      .Build();

      const auto reads = policy.GetReadComponents();
      const auto writes = policy.GetWriteComponents();
      CHECK(std::ranges::is_sorted(reads));
      CHECK(std::ranges::is_sorted(writes));
    }

    SUBCASE("Duplicate component across Query calls is inserted only once") {
      const AccessPolicy policy = AccessPolicyBuilder()
                                      .Query<const Position&>()
                                      .Query<const Position&, const Velocity&>()
                                      .Build();
      CHECK_EQ(policy.GetReadComponents().size(), 2);
    }

    SUBCASE("Value copy access (T) is treated as read") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position>().Build();
      CHECK_EQ(policy.GetReadComponents().size(), 1);
      CHECK(policy.GetWriteComponents().empty());
    }

    SUBCASE("Rvalue reference access (T&&) is treated as write") {
      const AccessPolicy policy =
          AccessPolicyBuilder().Query<Position&&>().Build();
      CHECK(policy.GetReadComponents().empty());
      CHECK_EQ(policy.GetWriteComponents().size(), 1);
    }
  }

  TEST_CASE("ecs::AccessPolicyBuilder::ReadResources") {
    SUBCASE("Single resource is added to the read set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera>().Build();

      CHECK_EQ(policy.GetReadResources().size(), 1);
      CHECK(policy.GetWriteResources().empty());
      CHECK(policy.HasReadResource(ResourceTypeIndex::From<Camera>()));
    }

    SUBCASE("Multiple resources are all added to the read set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<Camera, RenderSettings>().Build();
      CHECK_EQ(policy.GetReadResources().size(), 2);
    }

    SUBCASE("Thread-safe resources are silently ignored") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<ThreadSafeCounter>().Build();
      CHECK(policy.GetReadResources().empty());
    }

    SUBCASE("Explicit non-thread-safe resource is included") {
      const AccessPolicy policy =
          AccessPolicyBuilder().ReadResources<NonThreadSafeData>().Build();
      CHECK_EQ(policy.GetReadResources().size(), 1);
    }

    SUBCASE("Resources in the read set are sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .ReadResources<RenderQueue, Camera, RenderSettings>()
              .Build();
      const auto span = policy.GetReadResources();
      CHECK(std::ranges::is_sorted(span));
    }
  }

  TEST_CASE("ecs::AccessPolicyBuilder::WriteResources") {
    SUBCASE("Single resource is added to the write set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<RenderQueue>().Build();

      CHECK_EQ(policy.GetWriteResources().size(), 1);
      CHECK(policy.GetReadResources().empty());
      CHECK(policy.HasWriteResource(ResourceTypeIndex::From<RenderQueue>()));
    }

    SUBCASE("Multiple resources are all added to the write set") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<Camera, RenderQueue>().Build();
      CHECK_EQ(policy.GetWriteResources().size(), 2);
    }

    SUBCASE("Thread-safe resources are silently ignored") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<ThreadSafeCounter>().Build();
      CHECK(policy.GetWriteResources().empty());
    }

    SUBCASE("Explicit non-thread-safe resource is included") {
      const AccessPolicy policy =
          AccessPolicyBuilder().WriteResources<NonThreadSafeData>().Build();
      CHECK_EQ(policy.GetWriteResources().size(), 1);
    }

    SUBCASE("Resources in the write set are sorted") {
      const AccessPolicy policy =
          AccessPolicyBuilder()
              .WriteResources<RenderQueue, Camera, RenderSettings>()
              .Build();
      const auto span = policy.GetWriteResources();
      CHECK(std::ranges::is_sorted(span));
    }
  }
}
