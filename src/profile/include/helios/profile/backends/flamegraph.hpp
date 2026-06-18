#pragma once

#include <helios/cstring_view.hpp>
#include <helios/profile/backend.hpp>
#include <helios/profile/common.hpp>

#include <concurrentqueue/concurrentqueue.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace helios::profile {

/**
 * @brief Configuration for the FlamegraphBackend.
 * @details Controls the output destination and flushing behavior of the
 * Chrome Tracing JSON backend.
 */
struct FlamegraphBackendConfig {
  /// @brief Default number of events accumulated before flushing.
  static constexpr size_t kDefaultFlushThreshold = 1024;

  /// File path where the JSON trace is written.
  std::filesystem::path output_path = "helios_flamegraph.json";

  /// Number of events before flushing to disk.
  /// A threshold of `0` holds all events in memory until shutdown.
  size_t flush_threshold = kDefaultFlushThreshold;
};

/**
 * @brief Profiling backend that emits a Chrome Tracing JSON file.
 * @details Collects zone begin/end, instant, and counter events into a
 * lock-free concurrent queue. Batched JSON output is written to disk
 * periodically (controlled by `FlamegraphBackendConfig::flush_threshold`) or
 * at shutdown. The resulting file is compatible with `about://tracing` in
 * Chromium-based browsers and https://ui.perfetto.dev.
 */
class FlamegraphBackend final : public Backend {
public:
  /**
   * @brief Constructs with a custom configuration.
   * @param config Configuration for output path and flush threshold
   */
  explicit FlamegraphBackend(FlamegraphBackendConfig config = {}) noexcept
      : config_(std::move(config)) {}

  FlamegraphBackend(const FlamegraphBackend&) = delete;
  FlamegraphBackend(FlamegraphBackend&&) = delete;
  ~FlamegraphBackend() noexcept override = default;

  FlamegraphBackend& operator=(const FlamegraphBackend&) = delete;
  FlamegraphBackend& operator=(FlamegraphBackend&&) = delete;

  /**
   * @brief Returns the per-zone storage size in bytes.
   * @return Byte count (aligned to `std::max_align_t`) needed for the internal
   * ZoneState.
   */
  [[nodiscard]] size_t ZoneStorageSize() const noexcept override;

  /**
   * @brief Opens the output file and writes the JSON array prologue.
   * @warning Not thread-safe.
   * Call during single-threaded startup after `Finalize()`.
   */
  void Startup() noexcept override;

  /**
   * @brief Flushes all remaining events, writes the JSON array epilogue,
   * and closes the output file.
   * @warning Not thread-safe.
   * Call during single-threaded shutdown.
   */
  void Shutdown() noexcept override;

  /**
   * @brief Records the start of an instrumented zone.
   * @details Stores the zone name and start timestamp in the per-zone
   * storage slice allocated by the Profiler.
   * @param spec Zone specification (name, source location, etc.)
   * @param storage Per-zone byte slice for this backend
   */
  void BeginZone(const ZoneSpec& spec,
                 std::span<std::byte> storage) noexcept override;

  /**
   * @brief Records the end of an instrumented zone and enqueues a
   * completed event.
   * @details Computes the duration, destroys the zone state, and pushes a
   * `CompleteEvent` into the lock-free queue. May trigger a batch flush if
   * the queue size exceeds `flush_threshold`.
   * @param storage Per-zone byte slice for this backend
   */
  void EndZone(std::span<std::byte> storage) noexcept override;

  /**
   * @brief Attaches a text annotation to the active zone.
   * @param storage Per-zone byte slice for this backend
   * @param text Text to attach
   */
  void ZoneText(std::span<std::byte> storage,
                std::string_view text) noexcept override;

  /**
   * @brief Attaches a numeric value to the active zone.
   * @param storage Per-zone byte slice for this backend
   * @param value Numeric value to attach
   */
  void ZoneValue(std::span<std::byte> storage,
                 uint64_t value) noexcept override;

  /**
   * @brief Overrides the name of the active zone.
   * @details The new name is used in the emitted JSON event.
   * @param storage Per-zone byte slice for this backend
   * @param name New zone name
   */
  void ZoneName(std::span<std::byte> storage,
                std::string_view name) noexcept override;

  /// @brief Enqueues an unnamed frame-mark instant event.
  void FrameMark() noexcept override;

  /**
   * @brief Enqueues a named frame-mark instant event.
   * @param name Frame marker name
   */
  void FrameMark(CStringView name) noexcept override;

  /**
   * @brief Enqueues a frame-region-start instant event.
   * @param name Frame region name
   */
  void FrameMarkStart(CStringView name) noexcept override;

  /**
   * @brief Enqueues a frame-region-end instant event.
   * @param name Frame region name
   */
  void FrameMarkEnd(CStringView name) noexcept override;

  /**
   * @brief Enqueues a message as an instant event.
   * @param text Message text
   * @param color Display color (unused in JSON output)
   */
  void Message(std::string_view text, uint32_t color) noexcept override;

  /**
   * @brief Thread name events are not emitted in the JSON format.
   * @param name Thread name
   */
  void SetThreadName(CStringView /*name*/) noexcept override {}

  /**
   * @brief Enqueues a counter (plot) event with the given value.
   * @details Emitted as a Chrome Tracing counter event (`ph: "C"`).
   * @param name Counter name
   * @param value Numeric value
   */
  void Plot(CStringView name, double value) noexcept override;

  /**
   * @brief Plot configuration is not represented in the JSON format.
   * @param name Counter name
   * @param type Plot format type
   * @param step Whether to use step interpolation
   * @param fill Whether to fill the area
   * @param color Display color
   */
  void PlotConfig(CStringView /*name*/, PlotFormat /*type*/, bool /*step*/,
                  bool /*fill*/, uint32_t /*color*/) noexcept override {}

  /**
   * @brief Memory allocation events are not emitted in the JSON format.
   * @param ptr Pointer to allocated memory
   * @param size Allocation size in bytes
   * @param name Optional pool name
   * @param depth Callstack depth
   * @param loc Source location of the allocation
   */
  void Alloc(const void* /*ptr*/, size_t /*size*/,
             std::optional<CStringView> /*name*/, int /*depth*/,
             std::source_location /*loc*/) noexcept override {}

  /**
   * @brief Memory deallocation events are not emitted in the JSON format.
   * @param ptr Pointer to freed memory
   * @param name Optional pool name
   * @param depth Callstack depth
   * @param loc Source location of the deallocation
   */
  void Free(const void* /*ptr*/, std::optional<CStringView> /*name*/,
            int /*depth*/, std::source_location /*loc*/) noexcept override {}

  /**
   * @brief Memory discard events are not emitted in the JSON format.
   * @param name Pool name
   */
  void MemoryDiscard(CStringView /*name*/) noexcept override {}

  /**
   * @brief Memory discard events are not emitted in the JSON format.
   * @param name Pool name
   * @param depth Callstack depth
   */
  void MemoryDiscard(CStringView /*name*/, int /*depth*/) noexcept override {}

  /**
   * @brief Returns the backend identifier.
   * @return `"flamegraph"`
   */
  [[nodiscard]] std::string_view Name() const noexcept override {
    return "flamegraph";
  }

private:
  /**
   * @brief Per-zone state stored in the zone storage slice.
   * @details Lives on the stack inside `ScopedZone::storage_` and is
   * placement-constructed / destroyed by `BeginZone` / `EndZone`.
   */
  struct ZoneState {
    std::string_view name;
    uint64_t start_ts = 0;
    std::string_view text;
    uint64_t value = 0;
    bool has_value = false;
    bool has_text = false;
  };

  /**
   * @brief A completed zone event ready for JSON emission.
   * @details Emitted as a Chrome Tracing complete event (`ph: "X"`) with
   * duration computed from `end_ts - start_ts`.
   */
  struct CompleteEvent {
    std::string name;
    uint64_t start_ts = 0;
    uint64_t end_ts = 0;
    uint64_t tid = 0;
    std::string text;
    uint64_t value = 0;
    bool has_value = false;
    bool has_text = false;
  };

  /**
   * @brief An instant event (frame marks, messages).
   * @details Emitted as a Chrome Tracing instant event (`ph: "i"`).
   */
  struct InstantEvent {
    std::string name;
    uint64_t ts = 0;
    uint64_t tid = 0;
    std::string scope;
  };

  /**
   * @brief A counter event (plot samples).
   * @details Emitted as a Chrome Tracing counter event (`ph: "C"`).
   */
  struct CounterEvent {
    std::string name;
    uint64_t ts = 0;
    double value = 0.0;
  };

  /// @brief Variant covering all event types stored in the lock-free queue.
  using FlamegraphEvent =
      std::variant<CompleteEvent, InstantEvent, CounterEvent>;

  /// @brief Lock-free concurrent queue type.
  using EventQueue = moodycamel::ConcurrentQueue<FlamegraphEvent>;

  /**
   * @brief Reinterprets a zone storage slice as ZoneState.
   * @param storage Byte span from the profiler
   * @return Pointer to the zone state
   */
  [[nodiscard]] static ZoneState* AsZoneState(
      std::span<std::byte> storage) noexcept {
    return std::launder(reinterpret_cast<ZoneState*>(storage.data()));
  }

  /**
   * @brief Returns the current time in microseconds since epoch.
   * @return Microsecond timestamp
   */
  [[nodiscard]] static uint64_t NowMicros() noexcept;

  /**
   * @brief Returns a numeric hash of the current thread id.
   * @return Thread identifier for JSON `tid` field
   */
  [[nodiscard]] static uint64_t ThreadId() noexcept;

  /**
   * @brief Writes a JSON-escaped string to the output stream.
   * @param os Output stream
   * @param str Raw string to escape and write
   */
  static void WriteJsonString(std::ostream& os, std::string_view str) noexcept;

  /**
   * @brief Writes a single event to the output file as a JSON object.
   * @param event Event to write
   */
  void WriteEvent(const FlamegraphEvent& event) noexcept;

  /**
   * @brief Dequeues and writes up to `count` events from the queue.
   * @details Acquires the flush lock to prevent concurrent writes to the
   * output file. If another thread is already flushing, returns
   * immediately.
   * @param count Maximum number of events to flush
   */
  void FlushBatch(size_t count) noexcept;

  /**
   * @brief Dequeues and writes all remaining events, then writes the JSON
   * array epilogue and closes the file.
   * @details Spins on the flush lock to ensure all concurrent flushes
   * complete before draining the queue. Only called from `Shutdown()`.
   */
  void FlushAll() noexcept;

  /**
   * @brief Called after each event enqueue to check if a flush is needed.
   * @details If `flush_threshold_ > 0` and the approximate queue size
   * reaches the threshold, triggers a batch flush.
   */
  void MaybeFlush() noexcept;

  FlamegraphBackendConfig config_;
  EventQueue event_queue_;
  std::ofstream output_file_;
  std::atomic_flag flush_lock_ = ATOMIC_FLAG_INIT;
  bool first_event_written_ = false;
  bool file_open_ = false;
  bool started_ = false;
};

}  // namespace helios::profile
