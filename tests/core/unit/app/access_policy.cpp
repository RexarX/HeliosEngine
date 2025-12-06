#include <doctest/doctest.h>

#include <helios/core/app/access_policy.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/resource.hpp>

#include <cstddef>
#include <string>

using namespace helios::app;
using namespace helios::ecs;

namespace {

struct Position {
  float x = 0.0F;
  float y = 0.0F;
  float z = 0.0F;
};

struct Velocity {
  float dx = 0.0F;
  float dy = 0.0F;
  float dz = 0.0F;
};

struct Health {
  int points = 100;
};

struct Name {
  std::string value;
};

struct Transform {
  float rotation = 0.0F;
};

struct Tag {};

struct GameTime {
  float delta_time = 0.016F;
};

struct PhysicsSettings {
  float gravity = 9.8F;
};

struct RenderSettings {
  bool vsync = true;
};

struct AudioSettings {
  float volume = 1.0F;
};

struct ThreadSafeResource {
  int value = 0;

  static constexpr bool ThreadSafe() noexcept { return true; }
};

}  // namespace

TEST_SUITE("app::AccessPolicy") {
  TEST_CASE("AccessPolicy: Default Construction") {
    const AccessPolicy policy;

    CHECK_FALSE(policy.HasQueries());
    CHECK_FALSE(policy.HasResources());
    CHECK(policy.GetQueries().empty());
    CHECK(policy.GetReadResources().empty());
    CHECK(policy.GetWriteResources().empty());
  }

  TEST_CASE("AccessPolicy: Query With Single Read Component") {
    const AccessPolicy policy = AccessPolicy().Query<const Position&>();

    CHECK(policy.HasQueries());
    CHECK_FALSE(policy.HasResources());
    CHECK_EQ(policy.GetQueries().size(), 1);

    const auto& query = policy.GetQueries()[0];
    CHECK_EQ(query.read_components.size(), 1);
    CHECK(query.write_components.empty());
  }

  TEST_CASE("AccessPolicy: Query With Single Write Component") {
    const AccessPolicy policy = AccessPolicy().Query<Position&>();

    CHECK(policy.HasQueries());
    CHECK_EQ(policy.GetQueries().size(), 1);

    const auto& query = policy.GetQueries()[0];
    CHECK(query.read_components.empty());
    CHECK_EQ(query.write_components.size(), 1);
  }

  TEST_CASE("AccessPolicy: Query With Multiple Components") {
    const AccessPolicy policy = AccessPolicy().Query<const Position&, Velocity&, const Health&>();

    CHECK(policy.HasQueries());
    CHECK_EQ(policy.GetQueries().size(), 1);

    const auto& query = policy.GetQueries()[0];
    CHECK_EQ(query.read_components.size(), 2);
    CHECK_EQ(query.write_components.size(), 1);
  }

  TEST_CASE("AccessPolicy: Multiple Queries") {
    const AccessPolicy policy = AccessPolicy().Query<const Position&>().Query<Velocity&, const Health&>();

    CHECK(policy.HasQueries());
    CHECK_EQ(policy.GetQueries().size(), 2);
  }

  TEST_CASE("AccessPolicy: Query Component Types Are Sorted") {
    const AccessPolicy policy = AccessPolicy().Query<const Health&, const Position&, const Velocity&>();

    const auto& query = policy.GetQueries()[0];
    CHECK_EQ(query.read_components.size(), 3);

    for (size_t i = 1; i < query.read_components.size(); ++i) {
      CHECK_LT(query.read_components[i - 1].type_id, query.read_components[i].type_id);
    }
  }

  TEST_CASE("AccessPolicy: ReadResources Single Resource") {
    const AccessPolicy policy = AccessPolicy().ReadResources<GameTime>();

    CHECK_FALSE(policy.HasQueries());
    CHECK(policy.HasResources());
    CHECK_EQ(policy.GetReadResources().size(), 1);
    CHECK(policy.GetWriteResources().empty());
  }

  TEST_CASE("AccessPolicy: ReadResources Multiple Resources") {
    const AccessPolicy policy = AccessPolicy().ReadResources<GameTime, PhysicsSettings, RenderSettings>();

    CHECK(policy.HasResources());
    CHECK_EQ(policy.GetReadResources().size(), 3);
    CHECK(policy.GetWriteResources().empty());
  }

  TEST_CASE("AccessPolicy: WriteResources Single Resource") {
    const AccessPolicy policy = AccessPolicy().WriteResources<GameTime>();

    CHECK(policy.HasResources());
    CHECK(policy.GetReadResources().empty());
    CHECK_EQ(policy.GetWriteResources().size(), 1);
  }

  TEST_CASE("AccessPolicy: WriteResources Multiple Resources") {
    const AccessPolicy policy = AccessPolicy().WriteResources<GameTime, PhysicsSettings>();

    CHECK(policy.HasResources());
    CHECK(policy.GetReadResources().empty());
    CHECK_EQ(policy.GetWriteResources().size(), 2);
  }

  TEST_CASE("AccessPolicy: ReadResources Are Sorted") {
    const AccessPolicy policy =
        AccessPolicy().ReadResources<RenderSettings, GameTime, PhysicsSettings, AudioSettings>();

    const auto read_resources = policy.GetReadResources();
    CHECK_EQ(read_resources.size(), 4);

    for (size_t i = 1; i < read_resources.size(); ++i) {
      CHECK_LT(read_resources[i - 1].type_id, read_resources[i].type_id);
    }
  }

  TEST_CASE("AccessPolicy: WriteResources Are Sorted") {
    const AccessPolicy policy =
        AccessPolicy().WriteResources<RenderSettings, GameTime, PhysicsSettings, AudioSettings>();

    const auto write_resources = policy.GetWriteResources();
    CHECK_EQ(write_resources.size(), 4);

    for (size_t i = 1; i < write_resources.size(); ++i) {
      CHECK_LT(write_resources[i - 1].type_id, write_resources[i].type_id);
    }
  }

  TEST_CASE("AccessPolicy: Combined Query And Resources") {
    const AccessPolicy policy =
        AccessPolicy().Query<const Position&, Velocity&>().ReadResources<GameTime>().WriteResources<PhysicsSettings>();

    CHECK(policy.HasQueries());
    CHECK(policy.HasResources());
    CHECK_EQ(policy.GetQueries().size(), 1);
    CHECK_EQ(policy.GetReadResources().size(), 1);
    CHECK_EQ(policy.GetWriteResources().size(), 1);
  }

  TEST_CASE("AccessPolicy: ThreadSafe Resources Are Ignored In ReadResources") {
    const AccessPolicy policy = AccessPolicy().ReadResources<ThreadSafeResource, GameTime>();

    CHECK_EQ(policy.GetReadResources().size(), 1);
  }

  TEST_CASE("AccessPolicy: ThreadSafe Resources Are Ignored In WriteResources") {
    const AccessPolicy policy = AccessPolicy().WriteResources<ThreadSafeResource, GameTime>();

    CHECK_EQ(policy.GetWriteResources().size(), 1);
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Write-Write Same Component") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>();
    const AccessPolicy policy2 = AccessPolicy().Query<Position&>();

    CHECK(policy1.HasQueryConflict(policy2));
    CHECK(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Write-Read Same Component") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>();
    const AccessPolicy policy2 = AccessPolicy().Query<const Position&>();

    CHECK(policy1.HasQueryConflict(policy2));
    CHECK(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Read-Read Same Component No Conflict") {
    const AccessPolicy policy1 = AccessPolicy().Query<const Position&>();
    const AccessPolicy policy2 = AccessPolicy().Query<const Position&>();

    CHECK_FALSE(policy1.HasQueryConflict(policy2));
    CHECK_FALSE(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Different Components No Conflict") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>();
    const AccessPolicy policy2 = AccessPolicy().Query<Velocity&>();

    CHECK_FALSE(policy1.HasQueryConflict(policy2));
    CHECK_FALSE(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Multiple Queries With Conflict") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>().Query<const Velocity&>();
    const AccessPolicy policy2 = AccessPolicy().Query<const Health&>().Query<Velocity&>();

    CHECK(policy1.HasQueryConflict(policy2));
    CHECK(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasQueryConflict Empty Policy") {
    const AccessPolicy policy1 = AccessPolicy();
    const AccessPolicy policy2 = AccessPolicy().Query<Position&>();

    CHECK_FALSE(policy1.HasQueryConflict(policy2));
    CHECK_FALSE(policy2.HasQueryConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Write-Write Same Resource") {
    const AccessPolicy policy1 = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().WriteResources<GameTime>();

    CHECK(policy1.HasResourceConflict(policy2));
    CHECK(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Write-Read Same Resource") {
    const AccessPolicy policy1 = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().ReadResources<GameTime>();

    CHECK(policy1.HasResourceConflict(policy2));
    CHECK(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Read-Read Same Resource No Conflict") {
    const AccessPolicy policy1 = AccessPolicy().ReadResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().ReadResources<GameTime>();

    CHECK_FALSE(policy1.HasResourceConflict(policy2));
    CHECK_FALSE(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Different Resources No Conflict") {
    const AccessPolicy policy1 = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().WriteResources<PhysicsSettings>();

    CHECK_FALSE(policy1.HasResourceConflict(policy2));
    CHECK_FALSE(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Multiple Resources With Conflict") {
    const AccessPolicy policy1 = AccessPolicy().WriteResources<GameTime, PhysicsSettings>();
    const AccessPolicy policy2 = AccessPolicy().ReadResources<PhysicsSettings, RenderSettings>();

    CHECK(policy1.HasResourceConflict(policy2));
    CHECK(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: HasResourceConflict Empty Policy") {
    const AccessPolicy policy1 = AccessPolicy();
    const AccessPolicy policy2 = AccessPolicy().WriteResources<GameTime>();

    CHECK_FALSE(policy1.HasResourceConflict(policy2));
    CHECK_FALSE(policy2.HasResourceConflict(policy1));
  }

  TEST_CASE("AccessPolicy: ConflictsWith Query Conflict") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>();
    const AccessPolicy policy2 = AccessPolicy().Query<const Position&>();

    CHECK(policy1.ConflictsWith(policy2));
    CHECK(policy2.ConflictsWith(policy1));
  }

  TEST_CASE("AccessPolicy: ConflictsWith Resource Conflict") {
    const AccessPolicy policy1 = AccessPolicy().WriteResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().ReadResources<GameTime>();

    CHECK(policy1.ConflictsWith(policy2));
    CHECK(policy2.ConflictsWith(policy1));
  }

  TEST_CASE("AccessPolicy: ConflictsWith Both Conflicts") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>().WriteResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().Query<const Position&>().ReadResources<GameTime>();

    CHECK(policy1.ConflictsWith(policy2));
    CHECK(policy2.ConflictsWith(policy1));
  }

  TEST_CASE("AccessPolicy: ConflictsWith No Conflicts") {
    const AccessPolicy policy1 = AccessPolicy().Query<Position&>().ReadResources<GameTime>();
    const AccessPolicy policy2 = AccessPolicy().Query<Velocity&>().ReadResources<PhysicsSettings>();

    CHECK_FALSE(policy1.ConflictsWith(policy2));
    CHECK_FALSE(policy2.ConflictsWith(policy1));
  }

  TEST_CASE("AccessPolicy: ConflictsWith Empty Policies") {
    const AccessPolicy policy1 = AccessPolicy();
    const AccessPolicy policy2 = AccessPolicy();

    CHECK_FALSE(policy1.ConflictsWith(policy2));
    CHECK_FALSE(policy2.ConflictsWith(policy1));
  }

  TEST_CASE("AccessPolicy: Complex Conflict Detection") {
    const AccessPolicy policy1 = AccessPolicy()
                                     .Query<Position&, const Velocity&>()
                                     .Query<Health&>()
                                     .ReadResources<GameTime>()
                                     .WriteResources<PhysicsSettings>();

    const AccessPolicy policy2 = AccessPolicy()
                                     .Query<const Position&, Velocity&>()
                                     .Query<const Health&>()
                                     .ReadResources<PhysicsSettings>()
                                     .WriteResources<RenderSettings>();

    CHECK(policy1.ConflictsWith(policy2));
  }

  TEST_CASE("AccessPolicy: No Duplicate Resources In Read") {
    const AccessPolicy policy = AccessPolicy().ReadResources<GameTime>().ReadResources<GameTime>();

    CHECK_EQ(policy.GetReadResources().size(), 1);
  }

  TEST_CASE("AccessPolicy: No Duplicate Resources In Write") {
    const AccessPolicy policy = AccessPolicy().WriteResources<GameTime>().WriteResources<GameTime>();

    CHECK_EQ(policy.GetWriteResources().size(), 1);
  }
}
