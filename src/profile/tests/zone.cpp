#include <doctest/doctest.h>

#include <helios/cstring_view.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

using namespace helios::profile;
using helios::CStringView;

namespace {

struct MockBackend final : public Backend {
  int begin_zone_calls = 0;
  int end_zone_calls = 0;
  int zone_text_calls = 0;
  int zone_value_calls = 0;
  int zone_name_calls = 0;

  std::string last_zone_name;
  std::string last_zone_text;
  uint64_t last_zone_value = 0;
  std::string last_zone_rename;

  [[nodiscard]] size_t ZoneStorageSize() const noexcept override {
    return sizeof(int);
  }

  void Startup() noexcept override {}
  void Shutdown() noexcept override {}

  void BeginZone(const ZoneSpec& spec,
                 std::span<std::byte> /*storage*/) noexcept override {
    ++begin_zone_calls;
    last_zone_name = spec.name;
  }

  void EndZone(std::span<std::byte> /*storage*/) noexcept override {
    ++end_zone_calls;
  }

  void ZoneText(std::span<std::byte> /*storage*/,
                std::string_view text) noexcept override {
    ++zone_text_calls;
    last_zone_text = text;
  }

  void ZoneValue(std::span<std::byte> /*storage*/,
                 uint64_t value) noexcept override {
    ++zone_value_calls;
    last_zone_value = value;
  }

  void ZoneName(std::span<std::byte> /*storage*/,
                std::string_view name) noexcept override {
    ++zone_name_calls;
    last_zone_rename = name;
  }

  void FrameMark() noexcept override {}
  void FrameMark(CStringView) noexcept override {}
  void FrameMarkStart(CStringView) noexcept override {}
  void FrameMarkEnd(CStringView) noexcept override {}
  void Message(std::string_view, uint32_t) noexcept override {}
  void SetThreadName(CStringView) noexcept override {}
  void Plot(CStringView, double) noexcept override {}
  void PlotConfig(CStringView, PlotFormat, bool, bool,
                  uint32_t) noexcept override {}
  void Alloc(const void*, size_t, std::optional<CStringView>, int,
             std::source_location) noexcept override {}
  void Free(const void*, std::optional<CStringView>, int,
            std::source_location) noexcept override {}
  void MemoryDiscard(CStringView) noexcept override {}
  void MemoryDiscard(CStringView, int) noexcept override {}
  [[nodiscard]] std::string_view Name() const noexcept override {
    return "mock";
  }
};

}  // namespace

TEST_SUITE("helios::profile::ScopedZone") {
  TEST_CASE("ScopedZone with active spec") {
    Profiler::Instance().Clear();

    SUBCASE("Active ScopedZone triggers BeginZone on backends") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "test_scope", .active = true};
        ScopedZone zone(spec);
        CHECK_EQ(mock.begin_zone_calls, 1);
        CHECK_EQ(mock.last_zone_name, "test_scope");
      }

      Profiler::Instance().Clear();
    }

    SUBCASE("Destructor triggers EndZone") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec);
      }
      CHECK_EQ(mock.end_zone_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("ScopedZone with inactive spec") {
    Profiler::Instance().Clear();

    SUBCASE("Inactive ScopedZone does not trigger BeginZone") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "inactive_zone", .active = false};
        ScopedZone zone(spec);
        CHECK_EQ(mock.begin_zone_calls, 0);
      }
      CHECK_EQ(mock.end_zone_calls, 0);

      Profiler::Instance().Clear();
    }

    SUBCASE("Two-arg ctor with active=false suppresses zone") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec, false);
        CHECK_EQ(mock.begin_zone_calls, 0);
      }

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("ScopedZone::SetText / SetValue / SetName") {
    Profiler::Instance().Clear();

    SUBCASE("SetText on active zone forwards to backends") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec);
        zone.SetText("hello");
        CHECK_EQ(mock.zone_text_calls, 1);
        CHECK_EQ(mock.last_zone_text, "hello");
      }

      Profiler::Instance().Clear();
    }

    SUBCASE("SetValue on active zone forwards to backends") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec);
        zone.SetValue(999);
        CHECK_EQ(mock.zone_value_calls, 1);
        CHECK_EQ(mock.last_zone_value, 999);
      }

      Profiler::Instance().Clear();
    }

    SUBCASE("SetName on active zone forwards to backends") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec);
        zone.SetName("renamed_zone");
        CHECK_EQ(mock.zone_name_calls, 1);
        CHECK_EQ(mock.last_zone_rename, "renamed_zone");
      }

      Profiler::Instance().Clear();
    }

    SUBCASE("SetText on inactive zone is a no-op") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone", .active = false};
        ScopedZone zone(spec);
        zone.SetText("should not appear");
        CHECK_EQ(mock.zone_text_calls, 0);
      }

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("ScopedZone::CurrentZoneStorage") {
    Profiler::Instance().Clear();

    SUBCASE("CurrentZoneStorage returns non-empty span inside active zone") {
      (void)Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "zone"};
        ScopedZone zone(spec);
        const auto storage = ScopedZone::CurrentZoneStorage();
        CHECK_FALSE(storage.empty());
      }

      Profiler::Instance().Clear();
    }

    SUBCASE("CurrentZoneStorage returns empty span outside any zone") {
      const auto storage = ScopedZone::CurrentZoneStorage();
      CHECK(storage.empty());
    }
  }

  TEST_CASE("ScopedZone nested zones") {
    Profiler::Instance().Clear();

    SUBCASE("Inner zone storage is restored after outer zone destruction") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      {
        const ZoneSpec spec{.name = "outer"};
        ScopedZone outer(spec);
        CHECK_EQ(mock.begin_zone_calls, 1);

        {
          const ZoneSpec inner_spec{.name = "inner"};
          ScopedZone inner(inner_spec);
          CHECK_EQ(mock.begin_zone_calls, 2);
        }
        CHECK_EQ(mock.end_zone_calls, 1);
      }
      CHECK_EQ(mock.end_zone_calls, 2);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("ScopedZone without finalization") {
    Profiler::Instance().Clear();

    SUBCASE("ScopedZone is inactive when profiler is not finalized") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      const ZoneSpec spec{.name = "early_zone"};
      ScopedZone zone(spec);
      CHECK_EQ(mock.begin_zone_calls, 0);

      const auto storage = ScopedZone::CurrentZoneStorage();
      CHECK(storage.empty());

      Profiler::Instance().Clear();
    }
  }
}
