#include <doctest/doctest.h>

#include <helios/ecs/schedule/dag.hpp>
#include <helios/ecs/system/system.hpp>

using namespace helios::ecs;

namespace {

[[nodiscard]] constexpr SystemId MakeSystemId(size_t id) noexcept {
  return SystemId{.id = id};
}

}  // namespace

TEST_SUITE("helios::ecs::Dag") {
  TEST_CASE("ecs::Dag::ctor") {
    SUBCASE("Default-constructed dag is empty") {
      const Dag dag;
      CHECK(dag.Nodes().empty());
    }

    SUBCASE("Move-constructed dag transfers state") {
      Dag source;
      source.AddNode(MakeSystemId(1));

      const Dag moved(std::move(source));

      CHECK_EQ(moved.Nodes().size(), 1);
    }
  }

  TEST_CASE("ecs::Dag::operator=") {
    SUBCASE("Move assignment transfers state") {
      Dag source;
      source.AddNode(MakeSystemId(1));

      Dag target;
      target = std::move(source);

      CHECK_EQ(target.Nodes().size(), 1);
    }
  }

  TEST_CASE("ecs::Dag::AddNode") {
    SUBCASE("Adding a node returns an increasing index") {
      Dag dag;

      const size_t index_a = dag.AddNode(MakeSystemId(1));
      const size_t index_b = dag.AddNode(MakeSystemId(2));
      const size_t index_c = dag.AddNode(MakeSystemId(3));

      CHECK_EQ(index_a, 0);
      CHECK_EQ(index_b, 1);
      CHECK_EQ(index_c, 2);
    }

    SUBCASE("Adding duplicate ids returns different indices") {
      Dag dag;

      const size_t index_a = dag.AddNode(MakeSystemId(1));
      const size_t index_b = dag.AddNode(MakeSystemId(1));

      CHECK_EQ(index_a, 0);
      CHECK_EQ(index_b, 1);
    }
  }

  TEST_CASE("ecs::Dag::AddEdge") {
    SUBCASE("Adding a valid edge returns success") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));

      const auto result = dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      CHECK(result.has_value());
    }

    SUBCASE("Adding a self-loop edge returns a cycle error") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));

      const auto result = dag.AddEdge(MakeSystemId(1), MakeSystemId(1));
      CHECK_FALSE(result.has_value());
      CHECK_EQ(result.error().kind, DagErrorKind::kCycleDetected);
    }

    SUBCASE("Adding an edge with unknown source returns unknown node error") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));

      const auto result = dag.AddEdge(MakeSystemId(99), MakeSystemId(1));
      CHECK_FALSE(result.has_value());
      CHECK_EQ(result.error().kind, DagErrorKind::kUnknownNode);
    }

    SUBCASE("Adding an edge with unknown target returns unknown node error") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));

      const auto result = dag.AddEdge(MakeSystemId(1), MakeSystemId(99));
      CHECK_FALSE(result.has_value());
      CHECK_EQ(result.error().kind, DagErrorKind::kUnknownNode);
    }
  }

  TEST_CASE("ecs::Dag::Sort") {
    SUBCASE("Sort on an empty dag returns an empty vector") {
      Dag dag;

      const auto result = dag.Sort();
      CHECK(result.has_value());
      CHECK(result->empty());
    }

    SUBCASE("Sort on a single node returns that node") {
      Dag dag;

      dag.AddNode(MakeSystemId(42));

      const auto result = dag.Sort();
      CHECK(result.has_value());
      CHECK_EQ(result->size(), 1);
      CHECK_EQ((*result)[0], MakeSystemId(42));
    }

    SUBCASE("Sort on a linear chain returns correct topological order") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      dag.AddNode(MakeSystemId(3));
      [[maybe_unused]] const auto edge1 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      [[maybe_unused]] const auto edge2 =
          dag.AddEdge(MakeSystemId(2), MakeSystemId(3));

      const auto result = dag.Sort();
      CHECK(result.has_value());
      CHECK_EQ(result->size(), 3);
      CHECK_EQ((*result)[0], MakeSystemId(1));
      CHECK_EQ((*result)[1], MakeSystemId(2));
      CHECK_EQ((*result)[2], MakeSystemId(3));
    }

    SUBCASE("Sort on a diamond chain returns valid topological order") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      dag.AddNode(MakeSystemId(3));
      dag.AddNode(MakeSystemId(4));
      [[maybe_unused]] const auto edge12 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      [[maybe_unused]] const auto edge13 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(3));
      [[maybe_unused]] const auto edge24 =
          dag.AddEdge(MakeSystemId(2), MakeSystemId(4));
      [[maybe_unused]] const auto edge34 =
          dag.AddEdge(MakeSystemId(3), MakeSystemId(4));

      const auto result = dag.Sort();
      CHECK(result.has_value());
      CHECK_EQ(result->size(), 4);

      const bool first_is_one = (*result)[0] == MakeSystemId(1);
      const bool last_is_four = (*result)[3] == MakeSystemId(4);
      CHECK(first_is_one);
      CHECK(last_is_four);
    }

    SUBCASE("Sort on a cycle returns a cycle error") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      dag.AddNode(MakeSystemId(3));
      [[maybe_unused]] const auto edge12 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      [[maybe_unused]] const auto edge23 =
          dag.AddEdge(MakeSystemId(2), MakeSystemId(3));
      [[maybe_unused]] const auto edge31 =
          dag.AddEdge(MakeSystemId(3), MakeSystemId(1));

      const auto result = dag.Sort();
      CHECK_FALSE(result.has_value());
      CHECK_EQ(result.error().kind, DagErrorKind::kCycleDetected);
    }
  }

  TEST_CASE("ecs::Dag::IndexOf") {
    SUBCASE("IndexOf returns the correct index for an existing node") {
      Dag dag;

      dag.AddNode(MakeSystemId(10));
      dag.AddNode(MakeSystemId(20));

      const auto index = dag.IndexOf(MakeSystemId(20));
      CHECK(index.has_value());
      CHECK_EQ(*index, 1);
    }

    SUBCASE("IndexOf returns unknown node error for a missing node") {
      Dag dag;

      const auto index = dag.IndexOf(MakeSystemId(42));
      CHECK_FALSE(index.has_value());
      CHECK_EQ(index.error().kind, DagErrorKind::kUnknownNode);
    }
  }

  TEST_CASE("ecs::Dag::HasCycle") {
    SUBCASE("Empty dag has no cycle") {
      Dag dag;
      CHECK_FALSE(dag.HasCycle());
    }

    SUBCASE("Linear dag has no cycle") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      [[maybe_unused]] const auto edge =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));

      CHECK_FALSE(dag.HasCycle());
    }

    SUBCASE("Cyclic dag returns true") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      [[maybe_unused]] const auto edge_a =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      [[maybe_unused]] const auto edge_b =
          dag.AddEdge(MakeSystemId(2), MakeSystemId(1));

      CHECK(dag.HasCycle());
    }

    SUBCASE("Diamond dag has no cycle") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));
      dag.AddNode(MakeSystemId(3));
      dag.AddNode(MakeSystemId(4));
      [[maybe_unused]] const auto edge12 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(2));
      [[maybe_unused]] const auto edge13 =
          dag.AddEdge(MakeSystemId(1), MakeSystemId(3));
      [[maybe_unused]] const auto edge24 =
          dag.AddEdge(MakeSystemId(2), MakeSystemId(4));
      [[maybe_unused]] const auto edge34 =
          dag.AddEdge(MakeSystemId(3), MakeSystemId(4));

      CHECK_FALSE(dag.HasCycle());
    }
  }

  TEST_CASE("ecs::Dag::Nodes") {
    SUBCASE("Nodes returns empty span for empty dag") {
      const Dag dag;
      CHECK(dag.Nodes().empty());
    }

    SUBCASE("Nodes returns systems in insertion order") {
      Dag dag;

      dag.AddNode(MakeSystemId(1));
      dag.AddNode(MakeSystemId(2));

      const auto nodes = dag.Nodes();
      CHECK_EQ(nodes.size(), 2);
      CHECK_EQ(nodes[0], MakeSystemId(1));
      CHECK_EQ(nodes[1], MakeSystemId(2));
    }
  }
}
