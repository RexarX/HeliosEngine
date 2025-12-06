#include <doctest/doctest.h>

#include <helios/core/ecs/details/archetype.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

using namespace helios::ecs;
using namespace helios::ecs::details;

namespace {

// Predefined constexpr constants for reuse
constexpr std::array<ComponentTypeId, 3> kComponentTypes123 = {100, 200, 300};
constexpr std::array<ComponentTypeId, 2> kComponentTypes12 = {100, 200};
constexpr std::array<ComponentTypeId, 2> kComponentTypes34 = {300, 400};
constexpr std::array<ComponentTypeId, 3> kComponentTypes321 = {300, 100, 200};
constexpr std::array<ComponentTypeId, 3> kExpectedSorted = {100, 200, 300};
constexpr std::array<ComponentTypeId, 1> kSingleComponent = {100};
constexpr std::array<ComponentTypeId, 1> kMissingComponent = {400};
constexpr std::array<ComponentTypeId, 2> kPartialMatch = {100, 400};
constexpr std::array<ComponentTypeId, 3> kNoMatch = {400, 500, 600};
constexpr std::array<ComponentTypeId, 0> kEmptyComponents = {};

}  // namespace

TEST_SUITE("ecs::details::Archetype") {
  TEST_CASE("Archetype::ctor: construction") {
    std::vector<ComponentTypeId> component_types(kComponentTypes123.begin(), kComponentTypes123.end());
    Archetype archetype(component_types);

    CHECK(archetype.Empty());
    CHECK_EQ(archetype.EntityCount(), 0);
    CHECK_EQ(archetype.Entities().size(), 0);
    CHECK_NE(archetype.Hash(), 0);

    // Component types should be sorted
    auto stored_types = archetype.ComponentTypes();
    CHECK(std::ranges::is_sorted(stored_types));
  }

  TEST_CASE("Archetype::ctor: component types sorting") {
    std::vector<ComponentTypeId> unsorted_types(kComponentTypes321.begin(), kComponentTypes321.end());
    Archetype archetype(unsorted_types);

    auto stored_types = archetype.ComponentTypes();
    std::vector<ComponentTypeId> expected(kExpectedSorted.begin(), kExpectedSorted.end());

    CHECK_EQ(stored_types.size(), 3);
    CHECK(std::ranges::equal(stored_types, expected));
  }

  TEST_CASE("Archetype::AddEntity") {
    std::vector<ComponentTypeId> component_types(kComponentTypes12.begin(), kComponentTypes12.end());
    Archetype archetype(component_types);

    Entity entity1(42, 1);
    Entity entity2(43, 1);

    archetype.AddEntity(entity1);
    CHECK_EQ(archetype.EntityCount(), 1);
    CHECK_FALSE(archetype.Empty());
    CHECK(archetype.Contains(entity1));
    CHECK_FALSE(archetype.Contains(entity2));

    archetype.AddEntity(entity2);
    CHECK_EQ(archetype.EntityCount(), 2);
    CHECK(archetype.Contains(entity1));
    CHECK(archetype.Contains(entity2));

    auto entities = archetype.Entities();
    CHECK_EQ(entities.size(), 2);
    CHECK(std::ranges::find(entities, entity1) != entities.end());
    CHECK(std::ranges::find(entities, entity2) != entities.end());
  }

  TEST_CASE("Archetype::AddEntity: duplicate") {
    std::vector<ComponentTypeId> component_types(kSingleComponent.begin(), kSingleComponent.end());
    Archetype archetype(component_types);

    Entity entity(42, 1);

    archetype.AddEntity(entity);
    CHECK_EQ(archetype.EntityCount(), 1);

    // Adding same entity again should not increase count
    archetype.AddEntity(entity);
    CHECK_EQ(archetype.EntityCount(), 1);
    CHECK(archetype.Contains(entity));
  }

  TEST_CASE("Archetype::RemoveEntity") {
    std::vector<ComponentTypeId> component_types(kComponentTypes12.begin(), kComponentTypes12.end());
    Archetype archetype(component_types);

    Entity entity1(42, 1);
    Entity entity2(43, 1);
    Entity entity3(44, 1);

    archetype.AddEntity(entity1);
    archetype.AddEntity(entity2);
    archetype.AddEntity(entity3);
    CHECK_EQ(archetype.EntityCount(), 3);

    archetype.RemoveEntity(entity2);
    CHECK_EQ(archetype.EntityCount(), 2);
    CHECK(archetype.Contains(entity1));
    CHECK_FALSE(archetype.Contains(entity2));
    CHECK(archetype.Contains(entity3));

    // Remove all entities
    archetype.RemoveEntity(entity1);
    archetype.RemoveEntity(entity3);
    CHECK_EQ(archetype.EntityCount(), 0);
    CHECK(archetype.Empty());
  }

  TEST_CASE("Archetype::RemoveEntity: non-existent") {
    std::vector<ComponentTypeId> component_types(kSingleComponent.begin(), kSingleComponent.end());
    Archetype archetype(component_types);

    Entity entity1(42, 1);
    Entity entity2(43, 1);

    archetype.AddEntity(entity1);
    CHECK_EQ(archetype.EntityCount(), 1);

    // Removing non-existent entity should not crash or change count
    archetype.RemoveEntity(entity2);
    CHECK_EQ(archetype.EntityCount(), 1);
    CHECK(archetype.Contains(entity1));
  }

  TEST_CASE("Archetype::HasComponents") {
    std::vector<ComponentTypeId> component_types(kComponentTypes123.begin(), kComponentTypes123.end());
    Archetype archetype(component_types);

    // Test with single component
    CHECK(archetype.HasComponents(std::span<const ComponentTypeId>(kSingleComponent)));

    CHECK_FALSE(archetype.HasComponents(std::span<const ComponentTypeId>(kMissingComponent)));

    // Test with multiple components
    CHECK(archetype.HasComponents(std::span<const ComponentTypeId>(kComponentTypes12)));

    std::vector<ComponentTypeId> multiple_partial(kPartialMatch.begin(), kPartialMatch.end());
    CHECK_FALSE(archetype.HasComponents(multiple_partial));

    // Test with all components
    CHECK(archetype.HasComponents(component_types));

    // Test with empty list
    CHECK(archetype.HasComponents(std::span<const ComponentTypeId>(kEmptyComponents)));
  }

  TEST_CASE("Archetype::HasAnyComponents") {
    std::vector<ComponentTypeId> component_types(kComponentTypes123.begin(), kComponentTypes123.end());
    Archetype archetype(component_types);

    // Test with single matching component
    CHECK(archetype.HasAnyComponents(std::span<const ComponentTypeId>(kSingleComponent)));

    CHECK_FALSE(archetype.HasAnyComponents(std::span<const ComponentTypeId>(kMissingComponent)));

    // Test with partial match
    std::vector<ComponentTypeId> partial_match = {100, 400, 500};
    CHECK(archetype.HasAnyComponents(partial_match));

    // Test with no matches
    std::vector<ComponentTypeId> no_match(kNoMatch.begin(), kNoMatch.end());
    CHECK_FALSE(archetype.HasAnyComponents(no_match));

    // Test with empty list
    CHECK_FALSE(archetype.HasAnyComponents(std::span<const ComponentTypeId>(kEmptyComponents)));
  }

  TEST_CASE("Archetype::Hash: consistency") {
    std::vector<ComponentTypeId> types1(kComponentTypes123.begin(), kComponentTypes123.end());
    std::vector<ComponentTypeId> types2(kComponentTypes321.begin(), kComponentTypes321.end());  // Different order
    std::vector<ComponentTypeId> types3 = {100, 200, 400};                                      // Different components

    Archetype archetype1(types1);
    Archetype archetype2(types2);
    Archetype archetype3(types3);

    // Same component types should have same hash regardless of input order
    CHECK_EQ(archetype1.Hash(), archetype2.Hash());

    // Different component types should have different hash
    CHECK_NE(archetype1.Hash(), archetype3.Hash());
  }

  TEST_CASE("Archetype::ctor: move semantics") {
    std::vector<ComponentTypeId> component_types(kComponentTypes12.begin(), kComponentTypes12.end());
    Archetype archetype(component_types);

    Entity entity(42, 1);
    archetype.AddEntity(entity);

    size_t original_hash = archetype.Hash();

    // Move constructor
    Archetype moved_archetype = std::move(archetype);
    CHECK_EQ(moved_archetype.EntityCount(), 1);
    CHECK(moved_archetype.Contains(entity));
    CHECK_EQ(moved_archetype.Hash(), original_hash);

    // Original should be in valid but unspecified state
    // We can't make assumptions about the moved-from state
  }

  TEST_CASE("Archetypes::ctor: default construction") {
    Archetypes archetypes;

    CHECK_EQ(archetypes.ArchetypeCount(), 0);
  }

  TEST_CASE("Archetypes::UpdateEntityArchetype: single component") {
    Archetypes archetypes;
    Entity entity(42, 1);

    std::vector<ComponentTypeId> components(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity, components);

    CHECK_EQ(archetypes.ArchetypeCount(), 1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity));
    CHECK_EQ(archetype->EntityCount(), 1);
    CHECK(archetype->HasComponents(components));
  }

  TEST_CASE("Archetypes::UpdateEntityArchetype: multiple components") {
    Archetypes archetypes;
    Entity entity(42, 1);

    std::vector<ComponentTypeId> components(kComponentTypes123.begin(), kComponentTypes123.end());
    archetypes.UpdateEntityArchetype(entity, components);

    CHECK_EQ(archetypes.ArchetypeCount(), 1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity));
    CHECK(archetype->HasComponents(components));
  }

  TEST_CASE("Archetypes::UpdateEntityArchetype: change components") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Start with one set of components
    std::vector<ComponentTypeId> components1(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.UpdateEntityArchetype(entity, components1);

    const Archetype* archetype1 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype1 != nullptr);
    CHECK(archetype1->Contains(entity));

    // Change to different set of components
    std::vector<ComponentTypeId> components2(kComponentTypes34.begin(), kComponentTypes34.end());
    archetypes.UpdateEntityArchetype(entity, components2);

    CHECK_EQ(archetypes.ArchetypeCount(), 2);

    const Archetype* archetype2 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype2 != nullptr);
    CHECK(archetype2->Contains(entity));
    CHECK(archetype2->HasComponents(components2));

    // Entity should no longer be in first archetype
    CHECK_FALSE(archetype1->Contains(entity));
  }

  TEST_CASE("Archetypes::UpdateEntityArchetype: multiple entities same components") {
    Archetypes archetypes;
    Entity entity1(42, 1);
    Entity entity2(43, 1);
    Entity entity3(44, 1);

    std::vector<ComponentTypeId> components(kComponentTypes12.begin(), kComponentTypes12.end());

    archetypes.UpdateEntityArchetype(entity1, components);
    archetypes.UpdateEntityArchetype(entity2, components);
    archetypes.UpdateEntityArchetype(entity3, components);

    // Should only create one archetype
    CHECK_EQ(archetypes.ArchetypeCount(), 1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity1);
    REQUIRE(archetype != nullptr);
    CHECK_EQ(archetype->EntityCount(), 3);
    CHECK(archetype->Contains(entity1));
    CHECK(archetype->Contains(entity2));
    CHECK(archetype->Contains(entity3));

    // All entities should be in same archetype
    CHECK_EQ(archetypes.GetEntityArchetype(entity2), archetype);
    CHECK_EQ(archetypes.GetEntityArchetype(entity3), archetype);
  }

  TEST_CASE("Archetypes::UpdateEntityArchetype: multiple entities different components") {
    Archetypes archetypes;
    Entity entity1(42, 1);
    Entity entity2(43, 1);

    std::vector<ComponentTypeId> components1(kComponentTypes12.begin(), kComponentTypes12.end());
    std::vector<ComponentTypeId> components2(kComponentTypes34.begin(), kComponentTypes34.end());

    archetypes.UpdateEntityArchetype(entity1, components1);
    archetypes.UpdateEntityArchetype(entity2, components2);

    CHECK_EQ(archetypes.ArchetypeCount(), 2);

    const Archetype* archetype1 = archetypes.GetEntityArchetype(entity1);
    const Archetype* archetype2 = archetypes.GetEntityArchetype(entity2);

    REQUIRE(archetype1 != nullptr);
    REQUIRE(archetype2 != nullptr);
    CHECK_NE(archetype1, archetype2);

    CHECK(archetype1->Contains(entity1));
    CHECK_FALSE(archetype1->Contains(entity2));
    CHECK_FALSE(archetype2->Contains(entity1));
    CHECK(archetype2->Contains(entity2));
  }

  TEST_CASE("Archetypes::RemoveEntity") {
    Archetypes archetypes;
    Entity entity1(42, 1);
    Entity entity2(43, 1);

    std::vector<ComponentTypeId> components(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.UpdateEntityArchetype(entity1, components);
    archetypes.UpdateEntityArchetype(entity2, components);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity1);
    REQUIRE(archetype != nullptr);
    CHECK_EQ(archetype->EntityCount(), 2);

    archetypes.RemoveEntity(entity1);

    CHECK_EQ(archetype->EntityCount(), 1);
    CHECK_FALSE(archetype->Contains(entity1));
    CHECK(archetype->Contains(entity2));
    CHECK_EQ(archetypes.GetEntityArchetype(entity1), nullptr);
    CHECK_EQ(archetypes.GetEntityArchetype(entity2), archetype);
  }

  TEST_CASE("Archetypes::RemoveEntity: non-existent") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Should not crash when removing non-existent entity
    archetypes.RemoveEntity(entity);
    CHECK_EQ(archetypes.ArchetypeCount(), 0);
    CHECK_EQ(archetypes.GetEntityArchetype(entity), nullptr);
  }

  TEST_CASE("Archetypes::GetEntityArchetype: non-existent") {
    Archetypes archetypes;
    Entity entity(42, 1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity);
    CHECK_EQ(archetype, nullptr);
  }

  TEST_CASE("Archetypes::FindMatchingArchetypes") {
    Archetypes archetypes;

    // Create entities with different component combinations
    Entity entity1(1, 1);
    Entity entity2(2, 1);
    Entity entity3(3, 1);
    Entity entity4(4, 1);

    std::vector<ComponentTypeId> comp_100_200(kComponentTypes12.begin(), kComponentTypes12.end());
    std::vector<ComponentTypeId> comp_100_200_300(kComponentTypes123.begin(), kComponentTypes123.end());
    std::vector<ComponentTypeId> comp_200_300 = {200, 300};
    std::vector<ComponentTypeId> comp_100(kSingleComponent.begin(), kSingleComponent.end());

    archetypes.UpdateEntityArchetype(entity1, comp_100_200);      // Has 100, 200
    archetypes.UpdateEntityArchetype(entity2, comp_100_200_300);  // Has 100, 200, 300
    archetypes.UpdateEntityArchetype(entity3, comp_200_300);      // Has 200, 300
    archetypes.UpdateEntityArchetype(entity4, comp_100);          // Has 100

    // Find archetypes with component 100
    std::vector<ComponentTypeId> with_100(kSingleComponent.begin(), kSingleComponent.end());
    auto matching = archetypes.FindMatchingArchetypes(with_100);

    CHECK_EQ(matching.size(), 3);  // entity1, entity2, entity4 archetypes

    // Find archetypes with components 100 AND 200
    std::vector<ComponentTypeId> with_100_200(kComponentTypes12.begin(), kComponentTypes12.end());
    matching = archetypes.FindMatchingArchetypes(with_100_200);

    CHECK_EQ(matching.size(), 2);  // entity1, entity2 archetypes

    // Find archetypes with component 100 but WITHOUT 300
    std::vector<ComponentTypeId> without_300 = {300};
    matching = archetypes.FindMatchingArchetypes(with_100, without_300);

    CHECK_EQ(matching.size(), 2);  // entity1, entity4 archetypes (entity2 has 300)
  }

  TEST_CASE("Archetypes::FindMatchingArchetypes: empty results") {
    Archetypes archetypes;

    Entity entity(1, 1);
    constexpr std::array<ComponentTypeId, 2> kInitialComponents = {100, 200};
    archetypes.UpdateEntityArchetype(entity, kInitialComponents);

    // Find archetypes with non-existent component
    std::vector<ComponentTypeId> with_999 = {999};
    auto matching = archetypes.FindMatchingArchetypes(with_999);
    CHECK(matching.empty());

    // Find archetypes without existing component (should exclude all)
    std::vector<ComponentTypeId> without_100(kSingleComponent.begin(), kSingleComponent.end());
    matching = archetypes.FindMatchingArchetypes({}, without_100);
    CHECK(matching.empty());
  }

  TEST_CASE("ecs::details::Archetypes - FindMatchingArchetypes Empty Archetypes") {
    Archetypes archetypes;

    // Create archetype but remove all entities
    Entity entity(1, 1);
    std::vector<ComponentTypeId> temp_components(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.UpdateEntityArchetype(entity, temp_components);
    archetypes.RemoveEntity(entity);

    // Should not return empty archetypes
    std::vector<ComponentTypeId> with_100(kSingleComponent.begin(), kSingleComponent.end());
    auto matching = archetypes.FindMatchingArchetypes(with_100);
    CHECK(matching.empty());
  }

  TEST_CASE("ecs::details::Archetypes - Clear") {
    Archetypes archetypes;

    Entity entity1(1, 1);
    Entity entity2(2, 1);

    std::vector<ComponentTypeId> comp1(kComponentTypes12.begin(), kComponentTypes12.end());
    std::vector<ComponentTypeId> comp2(kComponentTypes34.begin(), kComponentTypes34.end());
    archetypes.UpdateEntityArchetype(entity1, comp1);
    archetypes.UpdateEntityArchetype(entity2, comp2);

    CHECK_EQ(archetypes.ArchetypeCount(), 2);
    CHECK_NE(archetypes.GetEntityArchetype(entity1), nullptr);
    CHECK_NE(archetypes.GetEntityArchetype(entity2), nullptr);

    archetypes.Clear();

    CHECK_EQ(archetypes.ArchetypeCount(), 0);
    CHECK_EQ(archetypes.GetEntityArchetype(entity1), nullptr);
    CHECK_EQ(archetypes.GetEntityArchetype(entity2), nullptr);
  }

  TEST_CASE("ecs::details::Archetypes - Stress Test") {
    Archetypes archetypes;
    constexpr size_t entity_count = 1000;
    constexpr size_t component_variety = 10;

    std::vector<Entity> entities;
    for (size_t i = 0; i < entity_count; ++i) {
      entities.emplace_back(static_cast<Entity::IndexType>(i), 1);
    }

    // Create entities with various component combinations
    for (size_t i = 0; i < entity_count; ++i) {
      std::vector<ComponentTypeId> components;

      // Each entity gets a unique combination based on its index
      for (size_t j = 0; j < component_variety; ++j) {
        if ((i % (j + 1)) == 0) {
          components.push_back(static_cast<ComponentTypeId>(j + 100));
        }
      }

      if (!components.empty()) {
        archetypes.UpdateEntityArchetype(entities[i], components);
      }
    }

    // Verify all entities are in correct archetypes
    for (size_t i = 0; i < entity_count; ++i) {
      std::vector<ComponentTypeId> expected_components;
      for (size_t j = 0; j < component_variety; ++j) {
        if ((i % (j + 1)) == 0) {
          expected_components.push_back(static_cast<ComponentTypeId>(j + 100));
        }
      }

      if (!expected_components.empty()) {
        const Archetype* archetype = archetypes.GetEntityArchetype(entities[i]);
        REQUIRE(archetype != nullptr);
        CHECK(archetype->Contains(entities[i]));
        CHECK(archetype->HasComponents(expected_components));
      } else {
        CHECK_EQ(archetypes.GetEntityArchetype(entities[i]), nullptr);
      }
    }

    // Test finding matching archetypes
    std::vector<ComponentTypeId> search_components = {100, 102};  // Components 0 and 2
    auto matching = archetypes.FindMatchingArchetypes(search_components);

    // Count entities that should match (divisible by both 1 and 3)
    size_t expected_matches = 0;
    for (size_t i = 0; i < entity_count; ++i) {
      if ((i % 1 == 0) && (i % 3 == 0)) {  // Has both components 100 and 102
        ++expected_matches;
      }
    }

    size_t actual_matches = 0;
    for (const auto& archetype_ref : matching) {
      actual_matches += archetype_ref.get().EntityCount();
    }

    CHECK_EQ(actual_matches, expected_matches);
  }

  // ==========================================================================
  // Archetype Edge Graph Tests
  // ==========================================================================

  TEST_CASE("Archetype::EdgeGraph: initial state") {
    std::vector<ComponentTypeId> component_types(kComponentTypes12.begin(), kComponentTypes12.end());
    Archetype archetype(component_types);

    CHECK_EQ(archetype.EdgeCount(), 0);
    CHECK_EQ(archetype.GetAddEdge(100), nullptr);
    CHECK_EQ(archetype.GetRemoveEdge(100), nullptr);
  }

  TEST_CASE("Archetype::EdgeGraph: set and get add edge") {
    std::vector<ComponentTypeId> comp1(kComponentTypes12.begin(), kComponentTypes12.end());
    std::vector<ComponentTypeId> comp2(kComponentTypes123.begin(), kComponentTypes123.end());

    Archetype archetype1(comp1);
    Archetype archetype2(comp2);

    // Set add edge: archetype1 + component 300 -> archetype2
    archetype1.SetAddEdge(300, &archetype2);

    CHECK_EQ(archetype1.EdgeCount(), 1);
    CHECK_EQ(archetype1.GetAddEdge(300), &archetype2);
    CHECK_EQ(archetype1.GetAddEdge(100), nullptr);     // Different component
    CHECK_EQ(archetype1.GetRemoveEdge(300), nullptr);  // Wrong operation type
  }

  TEST_CASE("Archetype::EdgeGraph: set and get remove edge") {
    std::vector<ComponentTypeId> comp1(kComponentTypes123.begin(), kComponentTypes123.end());
    std::vector<ComponentTypeId> comp2(kComponentTypes12.begin(), kComponentTypes12.end());

    Archetype archetype1(comp1);
    Archetype archetype2(comp2);

    // Set remove edge: archetype1 - component 300 -> archetype2
    archetype1.SetRemoveEdge(300, &archetype2);

    CHECK_EQ(archetype1.EdgeCount(), 1);
    CHECK_EQ(archetype1.GetRemoveEdge(300), &archetype2);
    CHECK_EQ(archetype1.GetRemoveEdge(100), nullptr);  // Different component
    CHECK_EQ(archetype1.GetAddEdge(300), nullptr);     // Wrong operation type
  }

  TEST_CASE("Archetype::EdgeGraph: multiple edges") {
    std::vector<ComponentTypeId> comp_base(kComponentTypes12.begin(), kComponentTypes12.end());
    std::vector<ComponentTypeId> comp_add_300 = {100, 200, 300};
    std::vector<ComponentTypeId> comp_add_400 = {100, 200, 400};
    std::vector<ComponentTypeId> comp_remove_100 = {200};

    Archetype archetype_base(comp_base);
    Archetype archetype_add_300(comp_add_300);
    Archetype archetype_add_400(comp_add_400);
    Archetype archetype_remove_100(comp_remove_100);

    // Set multiple edges
    archetype_base.SetAddEdge(300, &archetype_add_300);
    archetype_base.SetAddEdge(400, &archetype_add_400);
    archetype_base.SetRemoveEdge(100, &archetype_remove_100);

    CHECK_EQ(archetype_base.EdgeCount(), 3);
    CHECK_EQ(archetype_base.GetAddEdge(300), &archetype_add_300);
    CHECK_EQ(archetype_base.GetAddEdge(400), &archetype_add_400);
    CHECK_EQ(archetype_base.GetRemoveEdge(100), &archetype_remove_100);
  }

  TEST_CASE("Archetype::EdgeGraph: nullptr edge for remove to empty") {
    std::vector<ComponentTypeId> comp_single(kSingleComponent.begin(), kSingleComponent.end());
    Archetype archetype(comp_single);

    // Removing the only component results in no archetype (nullptr)
    archetype.SetRemoveEdge(100, nullptr);

    CHECK_EQ(archetype.EdgeCount(), 1);
    CHECK_EQ(archetype.GetRemoveEdge(100), nullptr);
  }

  TEST_CASE("Archetype::HasComponent: single component check") {
    std::vector<ComponentTypeId> component_types(kComponentTypes123.begin(), kComponentTypes123.end());
    Archetype archetype(component_types);

    CHECK(archetype.HasComponent(100));
    CHECK(archetype.HasComponent(200));
    CHECK(archetype.HasComponent(300));
    CHECK_FALSE(archetype.HasComponent(400));
    CHECK_FALSE(archetype.HasComponent(0));
  }

  TEST_CASE("Archetype::ComponentCount") {
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    std::vector<ComponentTypeId> comp3(kComponentTypes123.begin(), kComponentTypes123.end());

    Archetype archetype1(comp1);
    Archetype archetype3(comp3);

    CHECK_EQ(archetype1.ComponentCount(), 1);
    CHECK_EQ(archetype3.ComponentCount(), 3);
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentAdd: basic") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Start with single component
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity, comp1);

    const Archetype* archetype1 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype1 != nullptr);
    CHECK_EQ(archetype1->EntityCount(), 1);

    // Add component 200
    std::vector<ComponentTypeId> comp12(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.MoveEntityOnComponentAdd(entity, 200, comp12);

    const Archetype* archetype2 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype2 != nullptr);
    CHECK_NE(archetype1, archetype2);
    CHECK(archetype2->Contains(entity));
    CHECK(archetype2->HasComponents(comp12));
    CHECK_FALSE(archetype1->Contains(entity));
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentAdd: edge caching") {
    Archetypes archetypes;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    // Start both entities with same component
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity1, comp1);
    archetypes.UpdateEntityArchetype(entity2, comp1);

    // Move first entity - should create edge
    std::vector<ComponentTypeId> comp12(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.MoveEntityOnComponentAdd(entity1, 200, comp12);

    // Move second entity - should use cached edge
    archetypes.MoveEntityOnComponentAdd(entity2, 200, comp12);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity1);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity1));
    CHECK(archetype->Contains(entity2));
    CHECK_EQ(archetype->EntityCount(), 2);

    // Check that edge was cached (TotalEdgeCount should include the edge)
    CHECK_GE(archetypes.TotalEdgeCount(), 1);
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentRemove: basic") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Start with two components
    std::vector<ComponentTypeId> comp12(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.UpdateEntityArchetype(entity, comp12);

    const Archetype* archetype1 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype1 != nullptr);

    // Remove component 200
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.MoveEntityOnComponentRemove(entity, 200, comp1);

    const Archetype* archetype2 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype2 != nullptr);
    CHECK_NE(archetype1, archetype2);
    CHECK(archetype2->Contains(entity));
    CHECK(archetype2->HasComponent(100));
    CHECK_FALSE(archetype2->HasComponent(200));
    CHECK_FALSE(archetype1->Contains(entity));
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentRemove: to empty") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Start with single component
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity, comp1);

    const Archetype* archetype1 = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype1 != nullptr);

    // Remove the only component
    std::vector<ComponentTypeId> empty;
    archetypes.MoveEntityOnComponentRemove(entity, 100, empty);

    // Entity should have no archetype now
    CHECK_EQ(archetypes.GetEntityArchetype(entity), nullptr);
    CHECK_FALSE(archetype1->Contains(entity));
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentRemove: edge caching") {
    Archetypes archetypes;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    // Start both entities with same components
    std::vector<ComponentTypeId> comp12(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.UpdateEntityArchetype(entity1, comp12);
    archetypes.UpdateEntityArchetype(entity2, comp12);

    // Move first entity - should create edge
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.MoveEntityOnComponentRemove(entity1, 200, comp1);

    // Move second entity - should use cached edge
    archetypes.MoveEntityOnComponentRemove(entity2, 200, comp1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity1);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity1));
    CHECK(archetype->Contains(entity2));
    CHECK_EQ(archetype->EntityCount(), 2);

    // Check that edge was cached
    CHECK_GE(archetypes.TotalEdgeCount(), 1);
  }

  TEST_CASE("Archetypes::MoveEntityOnComponentAdd: first component") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Entity starts with no archetype
    CHECK_EQ(archetypes.GetEntityArchetype(entity), nullptr);

    // Add first component
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.MoveEntityOnComponentAdd(entity, 100, comp1);

    const Archetype* archetype = archetypes.GetEntityArchetype(entity);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity));
    CHECK(archetype->HasComponent(100));
  }

  TEST_CASE("Archetypes::TotalEdgeCount") {
    Archetypes archetypes;
    Entity entity1(1, 1);
    Entity entity2(2, 1);

    CHECK_EQ(archetypes.TotalEdgeCount(), 0);

    // Create archetype and add entities
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity1, comp1);

    // Move entity to create edge
    std::vector<ComponentTypeId> comp12(kComponentTypes12.begin(), kComponentTypes12.end());
    archetypes.MoveEntityOnComponentAdd(entity1, 200, comp12);

    // Should have at least one cached edge now
    CHECK_GE(archetypes.TotalEdgeCount(), 1);
  }

  TEST_CASE("Archetypes::GetEntityArchetypeMutable") {
    Archetypes archetypes;
    Entity entity(42, 1);

    // Non-existent entity
    CHECK_EQ(archetypes.GetEntityArchetypeMutable(entity), nullptr);

    // Add entity to archetype
    std::vector<ComponentTypeId> comp1(kSingleComponent.begin(), kSingleComponent.end());
    archetypes.UpdateEntityArchetype(entity, comp1);

    Archetype* archetype = archetypes.GetEntityArchetypeMutable(entity);
    REQUIRE(archetype != nullptr);
    CHECK(archetype->Contains(entity));

    // Verify mutable access works
    const Archetype* const_archetype = archetypes.GetEntityArchetype(entity);
    CHECK_EQ(archetype, const_archetype);
  }

  TEST_CASE("Archetypes::EdgeGraph: stress test") {
    Archetypes archetypes;
    constexpr size_t entity_count = 100;

    std::vector<Entity> entities;
    for (size_t i = 0; i < entity_count; ++i) {
      entities.emplace_back(static_cast<Entity::IndexType>(i), 1);
    }

    // Create entities with single component
    std::vector<ComponentTypeId> comp1 = {100};
    for (auto& entity : entities) {
      archetypes.UpdateEntityArchetype(entity, comp1);
    }

    // Add component 200 to all entities - should cache edge after first
    std::vector<ComponentTypeId> comp12 = {100, 200};
    for (auto& entity : entities) {
      archetypes.MoveEntityOnComponentAdd(entity, 200, comp12);
    }

    // All entities should be in same archetype
    const Archetype* archetype = archetypes.GetEntityArchetype(entities[0]);
    REQUIRE(archetype != nullptr);
    CHECK_EQ(archetype->EntityCount(), entity_count);

    for (const auto& entity : entities) {
      CHECK_EQ(archetypes.GetEntityArchetype(entity), archetype);
    }

    // Edge should be cached
    CHECK_GE(archetypes.TotalEdgeCount(), 1);
  }

}  // TEST_SUITE
