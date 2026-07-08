#include <doctest/doctest.h>

#include <helios/app/app.hpp>
#include <helios/ecs/ecs.hpp>

using namespace helios::app;

TEST_SUITE("helios::app::Executor") {
  TEST_CASE("app::Executor::kThreadSafe") {
    SUBCASE("Executor is a thread-safe resource") {
      CHECK_EQ(helios::ecs::IsResourceThreadSafe<Executor>(), true);
    }
  }

  TEST_CASE("app::Executor::operator*") {
    SUBCASE("Dereferences wrapped executor") {
      App app(2);
      const Executor executor{app.GetExecutor()};

      CHECK_EQ(&*executor, &app.GetExecutor());
    }
  }

  TEST_CASE("app::Executor::operator->") {
    SUBCASE("Accesses wrapped executor") {
      App app(2);
      const Executor executor{app.GetExecutor()};

      CHECK_EQ(executor->WorkerCount(), app.GetExecutor().WorkerCount());
    }
  }
}

TEST_SUITE("helios::app::ExecutorPlugin") {
  TEST_CASE("app::ExecutorPlugin::Build") {
    SUBCASE("Adds Executor resource on initialize") {
      App app(2);
      app.AddPlugins(ExecutorPlugin{});
      app.Initialize();

      CHECK_EQ(app.GetWorld().HasResource<Executor>(), true);
      CHECK_EQ(&*app.GetWorld().ReadResource<Executor>(), &app.GetExecutor());
    }

    SUBCASE("Adds Executor resource usable through AsyncRes") {
      size_t observed_worker_count = 0;

      struct ReadExecutor {
        size_t* observed_worker_count = nullptr;

        void operator()(helios::ecs::AsyncRes<Executor> executor) const {
          *observed_worker_count = (*executor)->WorkerCount();
        }
      };

      App app(2);
      app.AddPlugins(ExecutorPlugin{});
      app.AddSystem(kUpdate, ReadExecutor{.observed_worker_count =
                                              &observed_worker_count});
      app.Initialize();
      app.Update();

      CHECK_EQ(observed_worker_count, app.GetExecutor().WorkerCount());
    }
  }
}
