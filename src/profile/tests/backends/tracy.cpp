#include <doctest/doctest.h>

#include <helios/profile/backends/tracy.hpp>
#include <helios/profile/profile.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>

using namespace helios::profile;

TEST_SUITE("helios::profile::TracyBackend") {
  TEST_CASE("TracyBackend::Name") {
    SUBCASE("Name returns \"tracy\"") {
      const TracyBackend backend;
      CHECK_EQ(backend.Name(), "tracy");
    }

    SUBCASE("Name is accessible through base pointer") {
      const TracyBackend backend;
      const Backend& ref = backend;
      CHECK_EQ(ref.Name(), "tracy");
    }
  }

  TEST_CASE("TracyBackend::ZoneStorageSize") {
    SUBCASE("ZoneStorageSize returns a non-negative value") {
      const TracyBackend backend;
      CHECK_GE(backend.ZoneStorageSize(), 0);
    }

    SUBCASE("ZoneStorageSize fits within kZoneStorageBytes") {
      const TracyBackend backend;
      CHECK_LE(backend.ZoneStorageSize(), kZoneStorageBytes);
    }
  }

  TEST_CASE("TracyBackend zone operations") {
    SUBCASE("BeginZone and EndZone do not crash") {
      TracyBackend backend;
      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "test_tracy_zone"};

      backend.BeginZone(spec, storage);
      backend.EndZone(storage);

      CHECK(true);
    }

    SUBCASE("BeginZone with color and callstack depth does not crash") {
      TracyBackend backend;
      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{
          .name = "colored", .color = 0xFF00FF, .callstack_depth = 3};

      backend.BeginZone(spec, storage);
      backend.EndZone(storage);

      CHECK(true);
    }

    SUBCASE("ZoneText does not crash") {
      TracyBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneText(storage, "processing");
      CHECK(true);
    }

    SUBCASE("ZoneValue does not crash") {
      TracyBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneValue(storage, 99);
      CHECK(true);
    }

    SUBCASE("ZoneName does not crash") {
      TracyBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneName(storage, "new_name");
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend frame operations") {
    SUBCASE("FrameMark does not crash") {
      TracyBackend backend;
      backend.FrameMark();
      CHECK(true);
    }

    SUBCASE("FrameMark with name does not crash") {
      TracyBackend backend;
      backend.FrameMark("Render");
      CHECK(true);
    }

    SUBCASE("FrameMarkStart and FrameMarkEnd do not crash") {
      TracyBackend backend;
      backend.FrameMarkStart("Physics");
      backend.FrameMarkEnd("Physics");
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend message / thread") {
    SUBCASE("Message does not crash") {
      TracyBackend backend;
      backend.Message("Event!", 0x00FF00);
      CHECK(true);
    }

    SUBCASE("SetThreadName does not crash") {
      TracyBackend backend;
      backend.SetThreadName("Worker-1");
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend plot") {
    SUBCASE("Plot does not crash") {
      TracyBackend backend;
      backend.Plot("FPS", 60.0);
      CHECK(true);
    }

    SUBCASE("PlotConfig does not crash") {
      TracyBackend backend;
      backend.PlotConfig("FPS", PlotFormat::kNumber, true, false, 0xFF);
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend memory operations") {
    SUBCASE("Alloc does not crash") {
      TracyBackend backend;
      int dummy = 0;
      backend.Alloc(&dummy, 64, "Heap", 0, std::source_location::current());
      CHECK(true);
    }

    SUBCASE("Free does not crash") {
      TracyBackend backend;
      int dummy = 0;
      backend.Free(&dummy, std::nullopt, 0, std::source_location::current());
      CHECK(true);
    }

    SUBCASE("MemoryDiscard does not crash") {
      TracyBackend backend;
      backend.MemoryDiscard("Arena");
      backend.MemoryDiscard("Arena", 1);
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend::Startup / Shutdown") {
    SUBCASE("Startup does not crash") {
      TracyBackend backend;
      backend.Startup();
      CHECK(true);
    }

    SUBCASE("Shutdown does not crash") {
      TracyBackend backend;
      backend.Shutdown();
      CHECK(true);
    }

    SUBCASE("Startup then Shutdown does not crash") {
      TracyBackend backend;
      backend.Startup();
      backend.Shutdown();
      CHECK(true);
    }
  }

  TEST_CASE("TracyBackend in Profiler") {
    Profiler::Instance().Clear();

    SUBCASE("TracyBackend can be registered with Profiler") {
      Profiler::Instance().AddBackend<TracyBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 1);
      CHECK(Profiler::Instance().Contains<TracyBackend>());
      Profiler::Instance().Clear();
    }

    SUBCASE("TracyBackend can be retrieved by type") {
      auto& backend = Profiler::Instance().AddBackend<TracyBackend>();
      auto& retrieved = Profiler::Instance().Get<TracyBackend>();
      CHECK_EQ(&backend, &retrieved);
      Profiler::Instance().Clear();
    }

    SUBCASE("TracyBackend can be finalized with Profiler") {
      Profiler::Instance().AddBackend<TracyBackend>();
      Profiler::Instance().Finalize();
      CHECK(Profiler::Instance().Finalized());
      Profiler::Instance().Clear();
    }

    SUBCASE("TracyBackend receives zone events via Profiler dispatch") {
      Profiler::Instance().AddBackend<TracyBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "tracy_via_profiler"};
      Profiler::Instance().BeginZone(spec, storage);
      Profiler::Instance().EndZone(storage);

      Profiler::Instance().Clear();
    }
  }
}
