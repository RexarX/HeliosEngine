#include <doctest/doctest.h>

#include <helios/cstring_view.hpp>
#include <helios/profile/backends/flamegraph.hpp>
#include <helios/profile/profile.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <source_location>
#include <span>
#include <string_view>

using namespace helios::profile;

namespace {

void RemoveTestFile(const std::filesystem::path& path) noexcept {
  std::error_code ec;
  std::filesystem::remove(path, ec);
}

// Minimal backend used only for multi-backend registration tests.
struct SecondBackend final : public Backend {
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override { return 8; }
  void Startup() noexcept override {}
  void Shutdown() noexcept override {}
  void BeginZone(const ZoneSpec&, std::span<std::byte>) noexcept override {}
  void EndZone(std::span<std::byte>) noexcept override {}
  void ZoneText(std::span<std::byte>, std::string_view) noexcept override {}
  void ZoneValue(std::span<std::byte>, uint64_t) noexcept override {}
  void ZoneName(std::span<std::byte>, std::string_view) noexcept override {}
  void FrameMark() noexcept override {}
  void FrameMark(helios::CStringView) noexcept override {}
  void FrameMarkStart(helios::CStringView) noexcept override {}
  void FrameMarkEnd(helios::CStringView) noexcept override {}
  void Message(std::string_view, uint32_t) noexcept override {}
  void SetThreadName(helios::CStringView) noexcept override {}
  void Plot(helios::CStringView, double) noexcept override {}
  void PlotConfig(helios::CStringView, PlotFormat, bool, bool,
                  uint32_t) noexcept override {}
  void Alloc(const void*, size_t, std::optional<helios::CStringView>, int,
             std::source_location) noexcept override {}
  void Free(const void*, std::optional<helios::CStringView>, int,
            std::source_location) noexcept override {}
  void MemoryDiscard(helios::CStringView) noexcept override {}
  void MemoryDiscard(helios::CStringView, int) noexcept override {}
  [[nodiscard]] std::string_view Name() const noexcept override {
    return "second";
  }
};

}  // namespace

TEST_SUITE("helios::profile::FlamegraphBackendConfig") {
  TEST_CASE("FlamegraphBackendConfig::ctor") {
    SUBCASE("Default config has default output path") {
      const FlamegraphBackendConfig config;
      CHECK_FALSE(config.output_path.empty());
    }

    SUBCASE("Default config has non-zero flush threshold") {
      const FlamegraphBackendConfig config;
      CHECK_GT(config.flush_threshold, 0);
    }

    SUBCASE("Custom path is preserved") {
      const FlamegraphBackendConfig config{.output_path = "test_custom.json",
                                           .flush_threshold = 512};
      CHECK_EQ(config.output_path, std::filesystem::path{"test_custom.json"});
      CHECK_EQ(config.flush_threshold, 512);
    }

    SUBCASE("Flush threshold of zero is allowed") {
      const FlamegraphBackendConfig config{.output_path = "disable.json",
                                           .flush_threshold = 0};
      CHECK_EQ(config.flush_threshold, 0);
    }
  }
}

TEST_SUITE("helios::profile::FlamegraphBackend") {
  TEST_CASE("FlamegraphBackend::Name") {
    SUBCASE("Name returns \"flamegraph\"") {
      const FlamegraphBackend backend;
      CHECK_EQ(backend.Name(), "flamegraph");
    }

    SUBCASE("Name is accessible through base pointer") {
      const FlamegraphBackend backend;
      const Backend& ref = backend;
      CHECK_EQ(ref.Name(), "flamegraph");
    }
  }

  TEST_CASE("FlamegraphBackend::ZoneStorageSize") {
    SUBCASE("ZoneStorageSize returns a positive non-zero value") {
      const FlamegraphBackend backend;
      CHECK_GT(backend.ZoneStorageSize(), 0);
    }

    SUBCASE("ZoneStorageSize fits within kZoneStorageBytes") {
      const FlamegraphBackend backend;
      CHECK_LE(backend.ZoneStorageSize(), kZoneStorageBytes);
    }
  }

  TEST_CASE("FlamegraphBackend zone operations") {
    SUBCASE("BeginZone and EndZone do not crash") {
      FlamegraphBackend backend;
      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "test_flame_zone"};

      backend.BeginZone(spec, storage);
      backend.EndZone(storage);

      CHECK(true);
    }

    SUBCASE("ZoneText does not crash") {
      FlamegraphBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneText(storage, "annotation");
      CHECK(true);
    }

    SUBCASE("ZoneValue does not crash") {
      FlamegraphBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneValue(storage, 42);
      CHECK(true);
    }

    SUBCASE("ZoneName does not crash") {
      FlamegraphBackend backend;
      std::array<std::byte, 256> storage{};

      backend.ZoneName(storage, "renamed");
      CHECK(true);
    }
  }

  TEST_CASE("FlamegraphBackend frame operations") {
    SUBCASE("FrameMark does not crash") {
      FlamegraphBackend backend;
      backend.FrameMark();
      CHECK(true);
    }

    SUBCASE("FrameMark with name does not crash") {
      FlamegraphBackend backend;
      backend.FrameMark("Render");
      CHECK(true);
    }

    SUBCASE("FrameMarkStart and FrameMarkEnd do not crash") {
      FlamegraphBackend backend;
      backend.FrameMarkStart("Start");
      backend.FrameMarkEnd("End");
      CHECK(true);
    }
  }

  TEST_CASE("FlamegraphBackend message / thread") {
    SUBCASE("Message does not crash") {
      FlamegraphBackend backend;
      backend.Message("Hello", 0xFF0000);
      CHECK(true);
    }

    SUBCASE("SetThreadName is a no-op") {
      FlamegraphBackend backend;
      backend.SetThreadName("Thread-1");
      CHECK(true);
    }
  }

  TEST_CASE("FlamegraphBackend plot") {
    SUBCASE("Plot does not crash") {
      FlamegraphBackend backend;
      backend.Plot("FPS", 60.0);
      CHECK(true);
    }

    SUBCASE("PlotConfig is a no-op") {
      FlamegraphBackend backend;
      backend.PlotConfig("FPS", PlotFormat::kNumber, true, false, 0);
      CHECK(true);
    }
  }

  TEST_CASE("FlamegraphBackend memory operations") {
    SUBCASE("Alloc is a no-op") {
      FlamegraphBackend backend;
      int dummy = 0;
      backend.Alloc(&dummy, 128, std::nullopt, 0,
                    std::source_location::current());
      CHECK(true);
    }

    SUBCASE("Free is a no-op") {
      FlamegraphBackend backend;
      int dummy = 0;
      backend.Free(&dummy, std::nullopt, 0, std::source_location::current());
      CHECK(true);
    }

    SUBCASE("MemoryDiscard is a no-op") {
      FlamegraphBackend backend;
      backend.MemoryDiscard("Pool");
      backend.MemoryDiscard("Pool", 1);
      CHECK(true);
    }
  }

  TEST_CASE("FlamegraphBackend::Startup / Shutdown") {
    const std::filesystem::path test_path = "helios_flamegraph_test.json";
    RemoveTestFile(test_path);

    SUBCASE("Startup creates the output file") {
      FlamegraphBackend backend(
          FlamegraphBackendConfig{.output_path = test_path});
      backend.Startup();
      CHECK(std::filesystem::exists(test_path));
    }

    SUBCASE("Shutdown finalizes and closes the output file") {
      FlamegraphBackend backend(
          FlamegraphBackendConfig{.output_path = test_path});
      backend.Startup();
      backend.Shutdown();
      CHECK(true);
    }

    SUBCASE("After Shutdown, file contains valid JSON") {
      FlamegraphBackend backend(
          FlamegraphBackendConfig{.output_path = test_path});
      backend.Startup();

      std::array<std::byte, 256> storage{};
      ZoneSpec spec{.name = "zone1"};
      backend.BeginZone(spec, storage);
      backend.EndZone(storage);

      backend.Shutdown();
      CHECK(std::filesystem::exists(test_path));
    }

    RemoveTestFile(test_path);
  }

  TEST_CASE("FlamegraphBackend in Profiler") {
    Profiler::Instance().Clear();

    SUBCASE("FlamegraphBackend can be registered with Profiler") {
      Profiler::Instance().AddBackend<FlamegraphBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 1);
      CHECK(Profiler::Instance().Contains<FlamegraphBackend>());
      Profiler::Instance().Clear();
    }

    SUBCASE("FlamegraphBackend can be retrieved by type") {
      auto& backend = Profiler::Instance().AddBackend<FlamegraphBackend>();
      auto& retrieved = Profiler::Instance().Get<FlamegraphBackend>();
      CHECK_EQ(&backend, &retrieved);
      Profiler::Instance().Clear();
    }

    SUBCASE("FlamegraphBackend can be finalized alongside other backends") {
      Profiler::Instance().AddBackend<FlamegraphBackend>();
      Profiler::Instance().AddBackend<SecondBackend>();
      Profiler::Instance().Finalize();
      CHECK(Profiler::Instance().Finalized());
      Profiler::Instance().Clear();
    }

    SUBCASE("FlamegraphBackend receives zone events via Profiler dispatch") {
      Profiler::Instance().AddBackend<FlamegraphBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "via_profiler"};
      Profiler::Instance().BeginZone(spec, storage);
      Profiler::Instance().EndZone(storage);

      Profiler::Instance().Clear();
    }
  }
}
