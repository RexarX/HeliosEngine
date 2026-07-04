#include <pch.hpp>

#include <helios/profile/backends/flamegraph.hpp>

#include <helios/assert.hpp>
#include <helios/profile/config.hpp>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <source_location>
#include <span>
#include <string_view>
#include <thread>
#include <variant>

namespace helios::profile {

namespace {

[[nodiscard]] constexpr size_t AlignUp(size_t value,
                                       size_t alignment) noexcept {
  return (value + alignment - 1) & ~(alignment - 1);
}

}  // namespace

size_t FlamegraphBackend::ZoneStorageSize() const noexcept {
  static_assert(AlignUp(sizeof(ZoneState), alignof(std::max_align_t)) <=
                    kZoneStorageBytes,
                "FlamegraphBackend zone state exceeds kZoneStorageBytes!");
  return AlignUp(sizeof(ZoneState), alignof(std::max_align_t));
}

void FlamegraphBackend::Startup() noexcept {
  output_file_.open(config_.output_path, std::ios::out | std::ios::trunc);
  if (!output_file_.is_open()) {
    return;
  }
  file_open_ = true;
  output_file_ << "[\n";
  started_ = true;
}

void FlamegraphBackend::Shutdown() noexcept {
  if (!started_) {
    return;
  }
  started_ = false;
  FlushAll();
}

void FlamegraphBackend::BeginZone(const ZoneSpec& spec,
                                  std::span<std::byte> storage) noexcept {
  auto* state = AsZoneState(storage);
  std::construct_at(state);
  state->name = spec.name.empty() ? spec.loc.function_name() : spec.name;
  state->start_ts = NowMicros();
}

void FlamegraphBackend::EndZone(std::span<std::byte> storage) noexcept {
  auto* state = AsZoneState(storage);
  const uint64_t end_ts = NowMicros();

  CompleteEvent event;
  event.name = std::string{state->name};
  event.start_ts = state->start_ts;
  event.end_ts = end_ts;
  event.tid = ThreadId();
  event.has_text = state->has_text;
  event.has_value = state->has_value;
  if (state->has_text) {
    event.text = std::string{state->text};
  }
  if (state->has_value) {
    event.value = state->value;
  }

  std::destroy_at(state);

  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::ZoneText(std::span<std::byte> storage,
                                 std::string_view text) noexcept {
  auto* state = AsZoneState(storage);
  state->text = text;
  state->has_text = true;
}

void FlamegraphBackend::ZoneValue(std::span<std::byte> storage,
                                  uint64_t value) noexcept {
  auto* state = AsZoneState(storage);
  state->value = value;
  state->has_value = true;
}

void FlamegraphBackend::ZoneName(std::span<std::byte> storage,
                                 std::string_view name) noexcept {
  auto* state = AsZoneState(storage);
  state->name = name;
}

void FlamegraphBackend::FrameMark() noexcept {
  InstantEvent event;
  event.name = "Frame";
  event.ts = NowMicros();
  event.tid = ThreadId();
  event.scope = "t";
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::FrameMark(CStringView name) noexcept {
  InstantEvent event;
  event.name = std::string{name.View()};
  event.ts = NowMicros();
  event.tid = ThreadId();
  event.scope = "t";
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::FrameMarkStart(CStringView name) noexcept {
  InstantEvent event;
  event.name = std::string{name.View()};
  event.ts = NowMicros();
  event.tid = ThreadId();
  event.scope = "t";
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::FrameMarkEnd(CStringView name) noexcept {
  InstantEvent event;
  event.name = std::string{name.View()};
  event.ts = NowMicros();
  event.tid = ThreadId();
  event.scope = "t";
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::Message(std::string_view text,
                                uint32_t /*color*/) noexcept {
  InstantEvent event;
  event.name = std::string{text};
  event.ts = NowMicros();
  event.tid = ThreadId();
  event.scope = "t";
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

void FlamegraphBackend::Plot(CStringView name, double value) noexcept {
  CounterEvent event;
  event.name = std::string{name.View()};
  event.ts = NowMicros();
  event.value = value;
  event_queue_.enqueue(std::move(event));
  MaybeFlush();
}

uint64_t FlamegraphBackend::NowMicros() noexcept {
  const auto now = std::chrono::high_resolution_clock::now();
  return static_cast<uint64_t>(
      std::chrono::time_point_cast<std::chrono::microseconds>(now)
          .time_since_epoch()
          .count());
}

uint64_t FlamegraphBackend::ThreadId() noexcept {
  return std::hash<std::thread::id>{}(std::this_thread::get_id());
}

void FlamegraphBackend::WriteJsonString(std::ostream& os,
                                        std::string_view str) noexcept {
  os << '"';
  for (const char ch : str) {
    switch (ch) {
      case '"':
        os << "\\\"";
        break;
      case '\\':
        os << "\\\\";
        break;
      case '\b':
        os << "\\b";
        break;
      case '\f':
        os << "\\f";
        break;
      case '\n':
        os << "\\n";
        break;
      case '\r':
        os << "\\r";
        break;
      case '\t':
        os << "\\t";
        break;
      default:
        os << ch;
        break;
    }
  }
  os << '"';
}

void FlamegraphBackend::WriteEvent(const FlamegraphEvent& event) noexcept {
  if (first_event_written_) {
    output_file_ << ",\n";
  }
  first_event_written_ = true;

  std::visit(
      [this](const auto& e) {
        using T = std::decay_t<decltype(e)>;

        if constexpr (std::same_as<T, CompleteEvent>) {
          output_file_ << "  {";
          output_file_ << R"("name":)";
          WriteJsonString(output_file_, e.name);
          output_file_ << R"(,"ph":"X","ts":)" << e.start_ts;
          output_file_ << R"(,"dur":)" << (e.end_ts - e.start_ts);
          output_file_ << R"(,"pid":0,"tid":)" << e.tid;

          if (e.has_text || e.has_value) {
            output_file_ << R"(,"args":{)";
            bool first_arg = true;
            if (e.has_text) {
              output_file_ << R"("text":)";
              WriteJsonString(output_file_, e.text);
              first_arg = false;
            }
            if (e.has_value) {
              if (!first_arg) {
                output_file_ << ',';
              }
              output_file_ << R"("value":)" << e.value;
            }
            output_file_ << '}';
          }

          output_file_ << '}';
        } else if constexpr (std::same_as<T, InstantEvent>) {
          output_file_ << "  {";
          output_file_ << R"("name":)";
          WriteJsonString(output_file_, e.name);
          output_file_ << R"(,"ph":"i","ts":)" << e.ts;
          output_file_ << R"(,"pid":0,"tid":)" << e.tid;
          output_file_ << R"(,"s":")" << e.scope << '"';
          output_file_ << '}';
        } else if constexpr (std::same_as<T, CounterEvent>) {
          output_file_ << "  {";
          output_file_ << R"("name":)";
          WriteJsonString(output_file_, e.name);
          output_file_ << R"(,"ph":"C","ts":)" << e.ts;
          output_file_ << R"(,"pid":0,"args":{)";
          WriteJsonString(output_file_, e.name);
          output_file_ << ':' << e.value;
          output_file_ << "}}";
        }
      },
      event);
}

void FlamegraphBackend::FlushBatch(size_t count) noexcept {
  if (!file_open_ || count == 0) {
    return;
  }

  if (flush_lock_.test_and_set(std::memory_order_acquire)) {
    return;
  }

  FlamegraphEvent event{std::in_place_type<CompleteEvent>};
  size_t written = 0;
  while (written < count && event_queue_.try_dequeue(event)) {
    WriteEvent(event);
    ++written;
  }

  flush_lock_.clear(std::memory_order_release);
}

void FlamegraphBackend::FlushAll() noexcept {
  if (!file_open_) {
    return;
  }

  while (flush_lock_.test_and_set(std::memory_order_acquire)) {
  }

  FlamegraphEvent event{std::in_place_type<CompleteEvent>};
  while (event_queue_.try_dequeue(event)) {
    WriteEvent(event);
  }

  output_file_ << "\n]\n";
  output_file_.close();
  file_open_ = false;

  flush_lock_.clear(std::memory_order_release);
}

void FlamegraphBackend::MaybeFlush() noexcept {
  if (config_.flush_threshold == 0 || !file_open_) {
    return;
  }

  if (event_queue_.size_approx() >= config_.flush_threshold) {
    FlushBatch(config_.flush_threshold);
  }
}

}  // namespace helios::profile
