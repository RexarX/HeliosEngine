#include <doctest/doctest.h>

#include <helios/cstring_view.hpp>
#include <helios/profile/profile.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <source_location>
#include <span>
#include <string>
#include <string_view>

using namespace helios::profile;
using helios::CStringView;

namespace {

// Minimal backend that records every call for verification.
struct MockBackend final : public Backend {
  int begin_zone_calls = 0;
  int end_zone_calls = 0;
  int zone_text_calls = 0;
  int zone_value_calls = 0;
  int zone_name_calls = 0;
  int frame_mark_calls = 0;
  int frame_mark_named_calls = 0;
  int frame_mark_start_calls = 0;
  int frame_mark_end_calls = 0;
  int message_calls = 0;
  int set_thread_name_calls = 0;
  int plot_calls = 0;
  int plot_config_calls = 0;
  int alloc_calls = 0;
  int free_calls = 0;
  int memory_discard_calls = 0;
  int memory_discard_depth_calls = 0;
  int startup_calls = 0;
  int shutdown_calls = 0;

  std::string last_zone_name;
  uint32_t last_zone_color = 0;
  int last_callstack_depth = 0;
  std::string last_zone_text;
  uint64_t last_zone_value = 0;
  std::string last_zone_rename;
  std::string last_frame_name;
  std::string last_message;
  uint32_t last_message_color = 0;
  std::string last_thread_name;
  std::string last_plot_name;
  double last_plot_value = 0.0;
  PlotFormat last_plot_format = PlotFormat::kNumber;
  bool last_plot_step = false;
  bool last_plot_fill = false;
  uint32_t last_plot_color = 0;
  size_t last_alloc_size = 0;
  std::string last_alloc_name;
  std::string last_discard_name;
  int last_discard_depth = 0;

  [[nodiscard]] size_t ZoneStorageSize() const noexcept override {
    return sizeof(int);
  }

  void Startup() noexcept override { ++startup_calls; }
  void Shutdown() noexcept override { ++shutdown_calls; }

  void BeginZone(const ZoneSpec& spec,
                 std::span<std::byte> /*storage*/) noexcept override {
    ++begin_zone_calls;
    last_zone_name = spec.name;
    last_zone_color = spec.color;
    last_callstack_depth = spec.callstack_depth;
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

  void FrameMark() noexcept override { ++frame_mark_calls; }

  void FrameMark(CStringView name) noexcept override {
    ++frame_mark_named_calls;
    last_frame_name = name;
  }

  void FrameMarkStart(CStringView name) noexcept override {
    ++frame_mark_start_calls;
    last_frame_name = name;
  }

  void FrameMarkEnd(CStringView name) noexcept override {
    ++frame_mark_end_calls;
    last_frame_name = name;
  }

  void Message(std::string_view text, uint32_t color) noexcept override {
    ++message_calls;
    last_message = text;
    last_message_color = color;
  }

  void SetThreadName(CStringView name) noexcept override {
    ++set_thread_name_calls;
    last_thread_name = name;
  }

  void Plot(CStringView name, double value) noexcept override {
    ++plot_calls;
    last_plot_name = name;
    last_plot_value = value;
  }

  void PlotConfig(CStringView name, PlotFormat type, bool step, bool fill,
                  uint32_t color) noexcept override {
    ++plot_config_calls;
    last_plot_name = name;
    last_plot_format = type;
    last_plot_step = step;
    last_plot_fill = fill;
    last_plot_color = color;
  }

  void Alloc(const void* /*ptr*/, size_t size, std::optional<CStringView> name,
             int depth, std::source_location /*loc*/) noexcept override {
    ++alloc_calls;
    last_alloc_size = size;
    last_alloc_name = name.has_value() ? std::string{name->View()} : "";
    last_callstack_depth = depth;
  }

  void Free(const void* /*ptr*/, std::optional<CStringView> /*name*/,
            int /*depth*/, std::source_location /*loc*/) noexcept override {
    ++free_calls;
  }

  void MemoryDiscard(CStringView name) noexcept override {
    ++memory_discard_calls;
    last_discard_name = name;
  }

  void MemoryDiscard(CStringView name, int depth) noexcept override {
    ++memory_discard_depth_calls;
    last_discard_name = name;
    last_discard_depth = depth;
  }

  [[nodiscard]] std::string_view Name() const noexcept override {
    return "mock";
  }
};

// Backend with larger zone storage for StorageOffset testing.
struct LargeBackend final : public Backend {
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override { return 128; }
  void BeginZone(const ZoneSpec&, std::span<std::byte>) noexcept override {}
  void EndZone(std::span<std::byte>) noexcept override {}
  void ZoneText(std::span<std::byte>, std::string_view) noexcept override {}
  void ZoneValue(std::span<std::byte>, uint64_t) noexcept override {}
  void ZoneName(std::span<std::byte>, std::string_view) noexcept override {}
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
    return "large";
  }
};

}  // namespace

TEST_SUITE("helios::profile::PlotFormat") {
  TEST_CASE("PlotFormat values") {
    SUBCASE("Enum members are distinct") {
      CHECK_NE(PlotFormat::kNumber, PlotFormat::kMemory);
      CHECK_NE(PlotFormat::kMemory, PlotFormat::kPercentage);
      CHECK_NE(PlotFormat::kPercentage, PlotFormat::kWatts);
    }

    SUBCASE("PlotFormat is copyable and assignable") {
      const auto a = PlotFormat::kNumber;
      const auto b = a;
      CHECK_EQ(a, b);

      auto c = PlotFormat::kMemory;
      c = PlotFormat::kWatts;
      CHECK_EQ(c, PlotFormat::kWatts);
    }
  }
}

TEST_SUITE("helios::profile::ZoneSpec") {
  TEST_CASE("ZoneSpec::ctor") {
    SUBCASE("Default-constructed ZoneSpec has empty name") {
      const ZoneSpec spec;
      CHECK(spec.name.empty());
    }

    SUBCASE("ZoneSpec with fields stores given values") {
      const auto loc = std::source_location::current();
      const ZoneSpec spec{.name = "test_zone",
                          .loc = loc,
                          .color = 0xFF0000,
                          .active = true,
                          .callstack_depth = 3};
      CHECK_EQ(spec.name, "test_zone");
      CHECK_EQ(spec.loc.line(), loc.line());
      CHECK_EQ(spec.color, 0xFF0000);
      CHECK(spec.active);
      CHECK_EQ(spec.callstack_depth, 3);
    }

    SUBCASE("ZoneSpec with color zero is valid") {
      const ZoneSpec spec{.name = "zero_color", .color = 0};
      CHECK_EQ(spec.color, 0);
      CHECK(spec.active);
    }

    SUBCASE("ZoneSpec with active=false") {
      const ZoneSpec spec{.name = "inactive", .active = false};
      CHECK_FALSE(spec.active);
    }

    SUBCASE("ZoneSpec with callstack_depth zero") {
      const ZoneSpec spec{.callstack_depth = 0};
      CHECK_EQ(spec.callstack_depth, 0);
    }
  }
}

TEST_SUITE("helios::profile::Backend") {
  TEST_CASE("Backend::ZoneStorageSize") {
    SUBCASE("MockBackend returns expected size") {
      const MockBackend backend;
      CHECK_EQ(backend.ZoneStorageSize(), sizeof(int));
    }

    SUBCASE("LargeBackend returns 128 bytes") {
      const LargeBackend backend;
      CHECK_EQ(backend.ZoneStorageSize(), 128);
    }
  }

  TEST_CASE("Backend::BeginZone / EndZone") {
    SUBCASE("BeginZone and EndZone are called in pairs") {
      MockBackend backend;
      ZoneSpec spec{.name = "zone1", .color = 0x00FF00};

      std::array<std::byte, 256> storage{};
      backend.BeginZone(spec, storage);
      CHECK_EQ(backend.begin_zone_calls, 1);
      CHECK_EQ(backend.last_zone_name, "zone1");
      CHECK_EQ(backend.last_zone_color, 0x00FF00);

      backend.EndZone(storage);
      CHECK_EQ(backend.end_zone_calls, 1);
    }

    SUBCASE("ZoneSpec callstack depth is forwarded") {
      MockBackend backend;
      ZoneSpec spec{.callstack_depth = 5};
      std::array<std::byte, 256> storage{};
      backend.BeginZone(spec, storage);
      CHECK_EQ(backend.last_callstack_depth, 5);
    }
  }

  TEST_CASE("Backend::zone annotations") {
    SUBCASE("ZoneText forwards text") {
      MockBackend backend;
      std::array<std::byte, 256> storage{};
      backend.ZoneText(storage, "processing");
      CHECK_EQ(backend.zone_text_calls, 1);
      CHECK_EQ(backend.last_zone_text, "processing");
    }

    SUBCASE("ZoneValue forwards numeric value") {
      MockBackend backend;
      std::array<std::byte, 256> storage{};
      backend.ZoneValue(storage, 42);
      CHECK_EQ(backend.zone_value_calls, 1);
      CHECK_EQ(backend.last_zone_value, 42);
    }

    SUBCASE("ZoneName forwards new name") {
      MockBackend backend;
      std::array<std::byte, 256> storage{};
      backend.ZoneName(storage, "renamed");
      CHECK_EQ(backend.zone_name_calls, 1);
      CHECK_EQ(backend.last_zone_rename, "renamed");
    }

    SUBCASE("Multiple annotations on same zone") {
      MockBackend backend;
      std::array<std::byte, 256> storage{};
      backend.ZoneText(storage, "text1");
      backend.ZoneValue(storage, 100);
      backend.ZoneName(storage, "name1");
      CHECK_EQ(backend.zone_text_calls, 1);
      CHECK_EQ(backend.zone_value_calls, 1);
      CHECK_EQ(backend.zone_name_calls, 1);
    }
  }

  TEST_CASE("Backend::frame operations") {
    SUBCASE("FrameMark is called") {
      MockBackend backend;
      backend.FrameMark();
      CHECK_EQ(backend.frame_mark_calls, 1);
    }

    SUBCASE("FrameMark with name forwards the name") {
      MockBackend backend;
      backend.FrameMark("Render");
      CHECK_EQ(backend.frame_mark_named_calls, 1);
      CHECK_EQ(backend.last_frame_name, "Render");
    }

    SUBCASE("FrameMarkStart forwards name") {
      MockBackend backend;
      backend.FrameMarkStart("Physics");
      CHECK_EQ(backend.frame_mark_start_calls, 1);
      CHECK_EQ(backend.last_frame_name, "Physics");
    }

    SUBCASE("FrameMarkEnd forwards name") {
      MockBackend backend;
      backend.FrameMarkEnd("Physics");
      CHECK_EQ(backend.frame_mark_end_calls, 1);
      CHECK_EQ(backend.last_frame_name, "Physics");
    }

    SUBCASE("Frame start/end pair uses independent counters") {
      MockBackend backend;
      backend.FrameMarkStart("A");
      backend.FrameMarkEnd("A");
      CHECK_EQ(backend.frame_mark_start_calls, 1);
      CHECK_EQ(backend.frame_mark_end_calls, 1);
    }
  }

  TEST_CASE("Backend::message / thread") {
    SUBCASE("Message forwards text and color") {
      MockBackend backend;
      backend.Message("Event!", 0x00FF00);
      CHECK_EQ(backend.message_calls, 1);
      CHECK_EQ(backend.last_message, "Event!");
      CHECK_EQ(backend.last_message_color, 0x00FF00);
    }

    SUBCASE("Message with zero color is valid") {
      MockBackend backend;
      backend.Message("msg", 0);
      CHECK_EQ(backend.message_calls, 1);
      CHECK_EQ(backend.last_message_color, 0);
    }

    SUBCASE("SetThreadName forwards name") {
      MockBackend backend;
      backend.SetThreadName("Worker-3");
      CHECK_EQ(backend.set_thread_name_calls, 1);
      CHECK_EQ(backend.last_thread_name, "Worker-3");
    }
  }

  TEST_CASE("Backend::plot") {
    SUBCASE("Plot forwards name and value") {
      MockBackend backend;
      backend.Plot("FPS", 60.0);
      CHECK_EQ(backend.plot_calls, 1);
      CHECK_EQ(backend.last_plot_name, "FPS");
      CHECK_EQ(doctest::Approx(backend.last_plot_value), 60.0);
    }

    SUBCASE("PlotConfig forwards all parameters") {
      MockBackend backend;
      backend.PlotConfig("Memory", PlotFormat::kMemory, true, false, 0xFF0000);
      CHECK_EQ(backend.plot_config_calls, 1);
      CHECK_EQ(backend.last_plot_format, PlotFormat::kMemory);
      CHECK(backend.last_plot_step);
      CHECK_FALSE(backend.last_plot_fill);
      CHECK_EQ(backend.last_plot_color, 0xFF0000);
    }
  }

  TEST_CASE("Backend::memory operations") {
    SUBCASE("Alloc forwards pointer, size, and name") {
      MockBackend backend;
      int dummy = 0;
      backend.Alloc(&dummy, 64, "Heap", 0, std::source_location::current());
      CHECK_EQ(backend.alloc_calls, 1);
      CHECK_EQ(backend.last_alloc_size, 64);
      CHECK_EQ(backend.last_alloc_name, "Heap");
    }

    SUBCASE("Alloc with nullopt name sets empty string") {
      MockBackend backend;
      int dummy = 0;
      backend.Alloc(&dummy, 32, std::nullopt, 0,
                    std::source_location::current());
      CHECK_EQ(backend.alloc_calls, 1);
      CHECK(backend.last_alloc_name.empty());
    }

    SUBCASE("Free is called") {
      MockBackend backend;
      int dummy = 0;
      backend.Free(&dummy, std::nullopt, 0, std::source_location::current());
      CHECK_EQ(backend.free_calls, 1);
    }

    SUBCASE("MemoryDiscard forwards name") {
      MockBackend backend;
      backend.MemoryDiscard("FrameArena");
      CHECK_EQ(backend.memory_discard_calls, 1);
      CHECK_EQ(backend.last_discard_name, "FrameArena");
    }

    SUBCASE("MemoryDiscard with depth forwards both") {
      MockBackend backend;
      backend.MemoryDiscard("Pool", 2);
      CHECK_EQ(backend.memory_discard_depth_calls, 1);
      CHECK_EQ(backend.last_discard_name, "Pool");
      CHECK_EQ(backend.last_discard_depth, 2);
    }
  }

  TEST_CASE("Backend::Startup / Shutdown") {
    SUBCASE("Startup is called once") {
      MockBackend backend;
      backend.Startup();
      CHECK_EQ(backend.startup_calls, 1);
    }

    SUBCASE("Shutdown is called once") {
      MockBackend backend;
      backend.Shutdown();
      CHECK_EQ(backend.shutdown_calls, 1);
    }

    SUBCASE("Startup and Shutdown are independent") {
      MockBackend backend;
      backend.Startup();
      backend.Shutdown();
      CHECK_EQ(backend.startup_calls, 1);
      CHECK_EQ(backend.shutdown_calls, 1);
    }
  }

  TEST_CASE("Backend::Name") {
    SUBCASE("MockBackend returns \"mock\"") {
      const MockBackend backend;
      CHECK_EQ(backend.Name(), "mock");
    }

    SUBCASE("Name is callable via base pointer") {
      const MockBackend backend;
      const Backend& ref = backend;
      CHECK_EQ(ref.Name(), "mock");
    }
  }
}

TEST_SUITE("helios::profile::Profiler") {
  TEST_CASE("Profiler::Instance") {
    SUBCASE("Instance returns the same pointer each time") {
      CHECK_EQ(&Profiler::Instance(), &Profiler::Instance());
    }
  }

  TEST_CASE("Profiler::ctor") {
    SUBCASE("Profiler starts with zero backends") {
      Profiler::Instance().Clear();
      CHECK_EQ(Profiler::Instance().BackendCount(), 0);
    }

    SUBCASE("Profiler starts not finalized") {
      Profiler::Instance().Clear();
      CHECK_FALSE(Profiler::Instance().Finalized());
    }
  }

  TEST_CASE("Profiler::AddBackend") {
    SUBCASE("AddBackend increases BackendCount") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 1);
      Profiler::Instance().Clear();
    }

    SUBCASE("AddBackend returns reference to the registered backend") {
      Profiler::Instance().Clear();
      auto& backend = Profiler::Instance().AddBackend<MockBackend>();
      CHECK_EQ(backend.Name(), "mock");
      Profiler::Instance().Clear();
    }

    SUBCASE("Multiple distinct backends can be registered") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().AddBackend<LargeBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 2);
      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Finalize") {
    SUBCASE("Finalize sets finalized flag") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      CHECK(Profiler::Instance().Finalized());
      Profiler::Instance().Clear();
    }

    SUBCASE("Finalize without backends succeeds") {
      Profiler::Instance().Clear();
      Profiler::Instance().Finalize();
      CHECK(Profiler::Instance().Finalized());
      Profiler::Instance().Clear();
    }

    SUBCASE("Finalize computes storage offsets") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      CHECK_EQ(Profiler::Instance().StorageOffset<MockBackend>(), 0);
      Profiler::Instance().Clear();
    }

    SUBCASE("Second backend gets offset after first") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().AddBackend<LargeBackend>();
      Profiler::Instance().Finalize();

      const size_t mock_offset =
          Profiler::Instance().StorageOffset<MockBackend>();
      const size_t large_offset =
          Profiler::Instance().StorageOffset<LargeBackend>();
      CHECK(mock_offset < large_offset);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Clear") {
    SUBCASE("Clear resets BackendCount to zero") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      Profiler::Instance().Clear();
      CHECK_EQ(Profiler::Instance().BackendCount(), 0);
    }

    SUBCASE("Clear resets finalized flag") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      Profiler::Instance().Clear();
      CHECK_FALSE(Profiler::Instance().Finalized());
    }

    SUBCASE("After Clear, new backends can be added") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 1);
      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Get / TryGet") {
    SUBCASE("Get returns registered backend") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      auto& backend = Profiler::Instance().Get<MockBackend>();
      CHECK_EQ(backend.Name(), "mock");
      Profiler::Instance().Clear();
    }

    SUBCASE("TryGet returns nullptr for unregistered backend") {
      Profiler::Instance().Clear();
      CHECK_EQ(Profiler::Instance().TryGet<MockBackend>(), nullptr);
      Profiler::Instance().Clear();
    }

    SUBCASE("TryGet returns pointer to registered backend") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      auto* ptr = Profiler::Instance().TryGet<MockBackend>();
      REQUIRE(ptr != nullptr);
      CHECK_EQ(ptr->Name(), "mock");
      Profiler::Instance().Clear();
    }

    SUBCASE("TryGet by index returns nullptr for unregistered index") {
      Profiler::Instance().Clear();
      CHECK_EQ(
          Profiler::Instance().TryGet(BackendTypeIndex::From<MockBackend>()),
          nullptr);
    }
  }

  TEST_CASE("Profiler::Contains") {
    SUBCASE("Contains returns false for unregistered backend") {
      Profiler::Instance().Clear();
      CHECK_FALSE(Profiler::Instance().Contains<MockBackend>());
    }

    SUBCASE("Contains returns true after registration") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      CHECK(Profiler::Instance().Contains<MockBackend>());
      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::BackendCount") {
    SUBCASE("BackendCount is zero initially") {
      Profiler::Instance().Clear();
      CHECK_EQ(Profiler::Instance().BackendCount(), 0);
    }

    SUBCASE("BackendCount reflects number of registered backends") {
      Profiler::Instance().Clear();
      Profiler::Instance().AddBackend<MockBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 1);
      Profiler::Instance().AddBackend<LargeBackend>();
      CHECK_EQ(Profiler::Instance().BackendCount(), 2);
      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Startup / Shutdown") {
    SUBCASE("Startup calls Startup on all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      Profiler::Instance().Startup();
      CHECK_EQ(mock.startup_calls, 1);
      Profiler::Instance().Clear();
    }

    SUBCASE("Shutdown calls Shutdown on all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();
      Profiler::Instance().Shutdown();
      CHECK_EQ(mock.shutdown_calls, 1);
      Profiler::Instance().Clear();
    }

    SUBCASE("Startup is no-op when not finalized") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Startup();
      CHECK_EQ(mock.startup_calls, 0);
      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::BeginZone / EndZone") {
    SUBCASE("BeginZone dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "zone", .color = 0xFF};
      Profiler::Instance().BeginZone(spec, storage);
      CHECK_EQ(mock.begin_zone_calls, 1);
      CHECK_EQ(mock.last_zone_name, "zone");
      CHECK_EQ(mock.last_zone_color, 0xFF);

      Profiler::Instance().Clear();
    }

    SUBCASE("EndZone dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      Profiler::Instance().EndZone(storage);
      CHECK_EQ(mock.end_zone_calls, 1);

      Profiler::Instance().Clear();
    }

    SUBCASE("BeginZone and EndZone fan out to multiple backends") {
      Profiler::Instance().Clear();
      auto& a = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().AddBackend<LargeBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      const ZoneSpec spec{.name = "multi"};
      Profiler::Instance().BeginZone(spec, storage);
      Profiler::Instance().EndZone(storage);

      CHECK_EQ(a.begin_zone_calls, 1);
      CHECK_EQ(a.end_zone_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::ZoneText / ZoneValue / ZoneName") {
    SUBCASE("ZoneText dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      Profiler::Instance().ZoneText(storage, "msg");
      CHECK_EQ(mock.zone_text_calls, 1);
      CHECK_EQ(mock.last_zone_text, "msg");

      Profiler::Instance().Clear();
    }

    SUBCASE("ZoneValue dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      Profiler::Instance().ZoneValue(storage, 256);
      CHECK_EQ(mock.zone_value_calls, 1);
      CHECK_EQ(mock.last_zone_value, 256);

      Profiler::Instance().Clear();
    }

    SUBCASE("ZoneName dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      std::array<std::byte, 256> storage{};
      Profiler::Instance().ZoneName(storage, "new_name");
      CHECK_EQ(mock.zone_name_calls, 1);
      CHECK_EQ(mock.last_zone_rename, "new_name");

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::FrameMark") {
    SUBCASE("FrameMark dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().FrameMark();
      CHECK_EQ(mock.frame_mark_calls, 1);

      Profiler::Instance().Clear();
    }

    SUBCASE("FrameMark with name dispatches correctly") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().FrameMark("Render");
      CHECK_EQ(mock.frame_mark_named_calls, 1);
      CHECK_EQ(mock.last_frame_name, "Render");

      Profiler::Instance().Clear();
    }

    SUBCASE("FrameMarkStart and FrameMarkEnd dispatch correctly") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().FrameMarkStart("Physics");
      CHECK_EQ(mock.frame_mark_start_calls, 1);
      CHECK_EQ(mock.last_frame_name, "Physics");

      Profiler::Instance().FrameMarkEnd("Physics");
      CHECK_EQ(mock.frame_mark_end_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Message / SetThreadName") {
    SUBCASE("Message dispatches text and color") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().Message("Hello", 0xABCDEF);
      CHECK_EQ(mock.message_calls, 1);
      CHECK_EQ(mock.last_message, "Hello");
      CHECK_EQ(mock.last_message_color, 0xABCDEF);

      Profiler::Instance().Clear();
    }

    SUBCASE("SetThreadName dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().SetThreadName("MainThread");
      CHECK_EQ(mock.set_thread_name_calls, 1);
      CHECK_EQ(mock.last_thread_name, "MainThread");

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Plot / PlotConfig") {
    SUBCASE("Plot dispatches name and value") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().Plot("FrameTime", 16.6);
      CHECK_EQ(mock.plot_calls, 1);
      CHECK_EQ(mock.last_plot_name, "FrameTime");
      CHECK_EQ(doctest::Approx(mock.last_plot_value), 16.6);

      Profiler::Instance().Clear();
    }

    SUBCASE("PlotConfig dispatches all parameters") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().PlotConfig("FPS", PlotFormat::kNumber, true, false,
                                      0xFF);
      CHECK_EQ(mock.plot_config_calls, 1);
      CHECK_EQ(mock.last_plot_format, PlotFormat::kNumber);
      CHECK(mock.last_plot_step);
      CHECK_FALSE(mock.last_plot_fill);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::Alloc / Free") {
    SUBCASE("Alloc dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      int dummy = 0;
      Profiler::Instance().Alloc(&dummy, 128, "Heap", 1,
                                 std::source_location::current());
      CHECK_EQ(mock.alloc_calls, 1);
      CHECK_EQ(mock.last_alloc_size, 128);
      CHECK_EQ(mock.last_alloc_name, "Heap");

      Profiler::Instance().Clear();
    }

    SUBCASE("Free dispatches to all backends") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      int dummy = 0;
      Profiler::Instance().Free(&dummy, std::nullopt, 0,
                                std::source_location::current());
      CHECK_EQ(mock.free_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler::MemoryDiscard") {
    SUBCASE("MemoryDiscard without depth dispatches correctly") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().MemoryDiscard("FrameArena");
      CHECK_EQ(mock.memory_discard_calls, 1);
      CHECK_EQ(mock.last_discard_name, "FrameArena");

      Profiler::Instance().Clear();
    }

    SUBCASE("MemoryDiscard with depth dispatches correctly") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Profiler::Instance().MemoryDiscard("Pool", 3);
      CHECK_EQ(mock.memory_discard_depth_calls, 1);
      CHECK_EQ(mock.last_discard_name, "Pool");
      CHECK_EQ(mock.last_discard_depth, 3);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Profiler dispatch is no-op before finalize") {
    SUBCASE("BeginZone before finalize is no-op") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      std::array<std::byte, 256> storage{};
      Profiler::Instance().BeginZone(ZoneSpec{}, storage);
      CHECK_EQ(mock.begin_zone_calls, 0);
      Profiler::Instance().Clear();
    }

    SUBCASE("EndZone before finalize is no-op") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      std::array<std::byte, 256> storage{};
      Profiler::Instance().EndZone(storage);
      CHECK_EQ(mock.end_zone_calls, 0);
      Profiler::Instance().Clear();
    }

    SUBCASE("Message before finalize is no-op") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Message("test", 0);
      CHECK_EQ(mock.message_calls, 0);
      Profiler::Instance().Clear();
    }

    SUBCASE("FrameMark before finalize is no-op") {
      Profiler::Instance().Clear();
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().FrameMark();
      CHECK_EQ(mock.frame_mark_calls, 0);
      Profiler::Instance().Clear();
    }
  }
}

TEST_SUITE("helios::profile free functions") {
  TEST_CASE("FrameMark") {
    Profiler::Instance().Clear();

    SUBCASE("FrameMark dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      FrameMark();
      CHECK_EQ(mock.frame_mark_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("FrameMarkNamed") {
    Profiler::Instance().Clear();

    SUBCASE("FrameMarkNamed dispatches named frame mark") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      FrameMarkNamed("Render");
      CHECK_EQ(mock.frame_mark_named_calls, 1);
      CHECK_EQ(mock.last_frame_name, "Render");

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("FrameMarkStart / FrameMarkEnd") {
    Profiler::Instance().Clear();

    SUBCASE("FrameMarkStart and FrameMarkEnd dispatch correctly") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      FrameMarkStart("Start");
      CHECK_EQ(mock.frame_mark_start_calls, 1);

      FrameMarkEnd("End");
      CHECK_EQ(mock.frame_mark_end_calls, 1);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Message / SetThreadName") {
    Profiler::Instance().Clear();

    SUBCASE("Message dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Message("Hello World", 0xFF);
      CHECK_EQ(mock.message_calls, 1);
      CHECK_EQ(mock.last_message, "Hello World");
      CHECK_EQ(mock.last_message_color, 0xFF);

      Profiler::Instance().Clear();
    }

    SUBCASE("SetThreadName dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      SetThreadName("Worker");
      CHECK_EQ(mock.set_thread_name_calls, 1);
      CHECK_EQ(mock.last_thread_name, "Worker");

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Plot / PlotConfig") {
    Profiler::Instance().Clear();

    SUBCASE("Plot dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      Plot("Delta", 0.016);
      CHECK_EQ(mock.plot_calls, 1);
      CHECK_EQ(mock.last_plot_name, "Delta");
      CHECK_EQ(doctest::Approx(mock.last_plot_value), 0.016);

      Profiler::Instance().Clear();
    }

    SUBCASE("PlotConfig dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      PlotConfig("Mem", PlotFormat::kMemory, false, true, 0x00FF00);
      CHECK_EQ(mock.plot_config_calls, 1);
      CHECK_EQ(mock.last_plot_format, PlotFormat::kMemory);
      CHECK_FALSE(mock.last_plot_step);
      CHECK(mock.last_plot_fill);

      Profiler::Instance().Clear();
    }
  }

  TEST_CASE("Alloc / Free / MemoryDiscard") {
    Profiler::Instance().Clear();

    SUBCASE("Alloc dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      int dummy = 0;
      Alloc(&dummy, 256, "Pool", 0);
      CHECK_EQ(mock.alloc_calls, 1);
      CHECK_EQ(mock.last_alloc_size, 256);
      CHECK_EQ(mock.last_alloc_name, "Pool");

      Profiler::Instance().Clear();
    }

    SUBCASE("Free dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      int dummy = 0;
      Free(&dummy, std::nullopt, 0);
      CHECK_EQ(mock.free_calls, 1);

      Profiler::Instance().Clear();
    }

    SUBCASE("MemoryDiscard dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      MemoryDiscard("Arena");
      CHECK_EQ(mock.memory_discard_calls, 1);
      CHECK_EQ(mock.last_discard_name, "Arena");

      Profiler::Instance().Clear();
    }

    SUBCASE("MemoryDiscardS dispatches through Profiler") {
      auto& mock = Profiler::Instance().AddBackend<MockBackend>();
      Profiler::Instance().Finalize();

      MemoryDiscardS("Arena", 2);
      CHECK_EQ(mock.memory_discard_depth_calls, 1);
      CHECK_EQ(mock.last_discard_name, "Arena");
      CHECK_EQ(mock.last_discard_depth, 2);

      Profiler::Instance().Clear();
    }
  }
}
