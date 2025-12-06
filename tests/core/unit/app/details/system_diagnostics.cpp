#include <helios/core/app/details/system_diagnostics.hpp>

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

namespace helios::app::details {

namespace {

// Test components with custom names
struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
  static constexpr std::string_view GetName() noexcept { return "Position"; }
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
  static constexpr std::string_view GetName() noexcept { return "Velocity"; }
};

struct Health {
  int current = 100;
  int max = 100;
  static constexpr std::string_view GetName() noexcept { return "Health"; }
};

// Test resources with custom names
struct GameTime {
  float delta_time = 0.0F;
  float total_time = 0.0F;
  static constexpr std::string_view GetName() noexcept { return "GameTime"; }
};

struct PhysicsSettings {
  float gravity = -9.81F;
  static constexpr std::string_view GetName() noexcept { return "PhysicsSettings"; }
};

struct RenderSettings {
  int width = 1920;
  int height = 1080;
  static constexpr std::string_view GetName() noexcept { return "RenderSettings"; }
};

}  // namespace

TEST_SUITE("app::details::SystemDiagnostics") {
  TEST_CASE("SystemDiagnostics::AnalyzeComponentConflicts: no conflicts") {
    const AccessPolicy policy_a = AccessPolicy().Query<const Position&>();
    const AccessPolicy policy_b = AccessPolicy().Query<const Velocity&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    CHECK(conflicts.empty());
  }

  TEST_CASE("SystemDiagnostics::AnalyzeComponentConflicts: write-write conflict") {
    const AccessPolicy policy_a = AccessPolicy().Query<Position&>();
    const AccessPolicy policy_b = AccessPolicy().Query<Position&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].component_name, "Position");
    CHECK_EQ(conflicts[0].system_a_access, "write");
    CHECK_EQ(conflicts[0].system_b_access, "write");
    CHECK_FALSE(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeComponentConflicts: write-read conflict") {
    const AccessPolicy policy_a = AccessPolicy().Query<Position&>();
    const AccessPolicy policy_b = AccessPolicy().Query<const Position&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].component_name, "Position");
    CHECK_EQ(conflicts[0].system_a_access, "write");
    CHECK_EQ(conflicts[0].system_b_access, "read");
    CHECK(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeComponentConflicts: read-write conflict") {
    const AccessPolicy policy_a = AccessPolicy().Query<const Position&>();
    const AccessPolicy policy_b = AccessPolicy().Query<Position&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].component_name, "Position");
    CHECK_EQ(conflicts[0].system_a_access, "read");
    CHECK_EQ(conflicts[0].system_b_access, "write");
    CHECK(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeComponentConflicts: multiple conflicts") {
    const AccessPolicy policy_a = AccessPolicy().Query<Position&, Velocity&>();
    const AccessPolicy policy_b = AccessPolicy().Query<const Position&, Velocity&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    // Expect: Position (write-read), Velocity (write-write)
    CHECK_EQ(conflicts.size(), 2);

    bool found_position = false;
    bool found_velocity = false;
    for (const auto& conflict : conflicts) {
      if (conflict.component_name == "Position") {
        found_position = true;
        CHECK_EQ(conflict.system_a_access, "write");
        CHECK_EQ(conflict.system_b_access, "read");
      } else if (conflict.component_name == "Velocity") {
        found_velocity = true;
        CHECK_EQ(conflict.system_a_access, "write");
        CHECK_EQ(conflict.system_b_access, "write");
      }
    }
    CHECK(found_position);
    CHECK(found_velocity);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeResourceConflicts: no conflicts") {
    const AccessPolicy policy_a = AccessPolicy().ReadResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().ReadResources<PhysicsSettings>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    CHECK(conflicts.empty());
  }

  TEST_CASE("SystemDiagnostics::AnalyzeResourceConflicts: read-read no conflict") {
    const AccessPolicy policy_a = AccessPolicy().ReadResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().ReadResources<GameTime>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    CHECK(conflicts.empty());
  }

  TEST_CASE("SystemDiagnostics::AnalyzeResourceConflicts: write-write conflict") {
    const AccessPolicy policy_a = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().WriteResources<GameTime>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].resource_name, "GameTime");
    CHECK_EQ(conflicts[0].system_a_access, "write");
    CHECK_EQ(conflicts[0].system_b_access, "write");
    CHECK_FALSE(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeResourceConflicts: write-read conflict") {
    const AccessPolicy policy_a = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().ReadResources<GameTime>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].resource_name, "GameTime");
    CHECK_EQ(conflicts[0].system_a_access, "write");
    CHECK_EQ(conflicts[0].system_b_access, "read");
    CHECK(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::AnalyzeResourceConflicts: read-write conflict") {
    const AccessPolicy policy_a = AccessPolicy().ReadResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().WriteResources<GameTime>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    REQUIRE_EQ(conflicts.size(), 1);
    CHECK_EQ(conflicts[0].resource_name, "GameTime");
    CHECK_EQ(conflicts[0].system_a_access, "read");
    CHECK_EQ(conflicts[0].system_b_access, "write");
    CHECK(conflicts[0].read_write_conflict);
  }

  TEST_CASE("SystemDiagnostics::FormatComponentConflicts: empty conflicts") {
    const std::vector<SystemDiagnostics::ComponentConflict> conflicts;

    const std::string result = SystemDiagnostics::FormatComponentConflicts("SystemA", "SystemB", conflicts);

    CHECK(result.empty());
  }

  TEST_CASE("SystemDiagnostics::FormatComponentConflicts: with conflicts") {
    const AccessPolicy policy_a = AccessPolicy().Query<Position&>();
    const AccessPolicy policy_b = AccessPolicy().Query<const Position&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);
    const std::string result = SystemDiagnostics::FormatComponentConflicts("MovementSystem", "RenderSystem", conflicts);

    // Verify the output contains system names and component name
    CHECK(result.find("MovementSystem") != std::string::npos);
    CHECK(result.find("RenderSystem") != std::string::npos);
    CHECK(result.find("Position") != std::string::npos);
    CHECK(result.find("write") != std::string::npos);
    CHECK(result.find("read") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics::FormatResourceConflicts: empty conflicts") {
    const std::vector<SystemDiagnostics::ResourceConflict> conflicts;

    const std::string result = SystemDiagnostics::FormatResourceConflicts("SystemA", "SystemB", conflicts);

    CHECK(result.empty());
  }

  TEST_CASE("SystemDiagnostics::FormatResourceConflicts: with conflicts") {
    const AccessPolicy policy_a = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy_b = AccessPolicy().WriteResources<GameTime>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);
    const std::string result = SystemDiagnostics::FormatResourceConflicts("TimeSystem", "PhysicsSystem", conflicts);

    // Verify the output contains system names and resource name
    CHECK(result.find("TimeSystem") != std::string::npos);
    CHECK(result.find("PhysicsSystem") != std::string::npos);
    CHECK(result.find("GameTime") != std::string::npos);
    CHECK(result.find("write") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics::SummarizeAccessPolicy: empty policy") {
    const AccessPolicy policy;

    const std::string summary = SystemDiagnostics::SummarizeAccessPolicy(policy);

    CHECK(summary.find("No data access declared") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics::SummarizeAccessPolicy: query only") {
    const AccessPolicy policy = AccessPolicy().Query<const Position&, Velocity&>();

    const std::string summary = SystemDiagnostics::SummarizeAccessPolicy(policy);

    CHECK(summary.find("Queries") != std::string::npos);
    CHECK(summary.find("Position") != std::string::npos);
    CHECK(summary.find("Velocity") != std::string::npos);
    CHECK(summary.find("Read") != std::string::npos);
    CHECK(summary.find("Write") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics::SummarizeAccessPolicy: resources only") {
    const AccessPolicy policy = AccessPolicy().ReadResources<GameTime>().WriteResources<PhysicsSettings>();

    const std::string summary = SystemDiagnostics::SummarizeAccessPolicy(policy);

    CHECK(summary.find("Read Resources") != std::string::npos);
    CHECK(summary.find("Write Resources") != std::string::npos);
    CHECK(summary.find("GameTime") != std::string::npos);
    CHECK(summary.find("PhysicsSettings") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics::SummarizeAccessPolicy: full policy") {
    const AccessPolicy policy =
        AccessPolicy().Query<const Position&, Velocity&>().ReadResources<GameTime>().WriteResources<PhysicsSettings>();

    const std::string summary = SystemDiagnostics::SummarizeAccessPolicy(policy);

    CHECK(summary.find("Queries") != std::string::npos);
    CHECK(summary.find("Read Resources") != std::string::npos);
    CHECK(summary.find("Write Resources") != std::string::npos);
    CHECK(summary.find("Position") != std::string::npos);
    CHECK(summary.find("Velocity") != std::string::npos);
    CHECK(summary.find("GameTime") != std::string::npos);
    CHECK(summary.find("PhysicsSettings") != std::string::npos);
  }

  TEST_CASE("SystemDiagnostics: component names are preserved in conflicts") {
    // Test that component names from AccessPolicy are correctly used in diagnostics
    const AccessPolicy policy_a = AccessPolicy().Query<Position&, const Health&>();
    const AccessPolicy policy_b = AccessPolicy().Query<Position&, Health&>();

    const auto conflicts = SystemDiagnostics::AnalyzeComponentConflicts(policy_a, policy_b);

    // Should have conflicts for both Position (write-write) and Health (read-write)
    CHECK_GE(conflicts.size(), 2);

    bool found_position = false;
    bool found_health = false;
    for (const auto& conflict : conflicts) {
      if (conflict.component_name == "Position") {
        found_position = true;
      } else if (conflict.component_name == "Health") {
        found_health = true;
      }
    }
    CHECK(found_position);
    CHECK(found_health);
  }

  TEST_CASE("SystemDiagnostics: resource names are preserved in conflicts") {
    // Test that resource names from AccessPolicy are correctly used in diagnostics
    const AccessPolicy policy_a = AccessPolicy().WriteResources<GameTime, PhysicsSettings>();
    const AccessPolicy policy_b = AccessPolicy().WriteResources<GameTime>().ReadResources<PhysicsSettings>();

    const auto conflicts = SystemDiagnostics::AnalyzeResourceConflicts(policy_a, policy_b);

    CHECK_GE(conflicts.size(), 2);

    bool found_game_time = false;
    bool found_physics = false;
    for (const auto& conflict : conflicts) {
      if (conflict.resource_name == "GameTime") {
        found_game_time = true;
        CHECK_EQ(conflict.system_a_access, "write");
        CHECK_EQ(conflict.system_b_access, "write");
      } else if (conflict.resource_name == "PhysicsSettings") {
        found_physics = true;
        CHECK_EQ(conflict.system_a_access, "write");
        CHECK_EQ(conflict.system_b_access, "read");
      }
    }
    CHECK(found_game_time);
    CHECK(found_physics);
  }
}

}  // namespace helios::app::details
