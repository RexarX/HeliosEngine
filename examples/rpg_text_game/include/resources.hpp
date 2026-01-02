#pragma once

#include <helios/core_pch.hpp>

#include "components.hpp"

#include <helios/core/container/static_string.hpp>
#include <helios/core/ecs/entity.hpp>

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rpg {

// ============================================================================
// Game State Resource
// ============================================================================

enum class GameState : uint8_t {
  MainMenu,
  Exploring,
  InBattle,
  InDialog,
  InShop,
  GameOver,
  Victory,
  Paused,
};

struct GameStateResource {
  GameState current_state = GameState::MainMenu;
  GameState previous_state = GameState::MainMenu;
  bool should_quit = false;
  int turn_number = 0;

  void TransitionTo(GameState new_state) noexcept {
    previous_state = current_state;
    current_state = new_state;
  }

  [[nodiscard]] constexpr bool InCombat() const noexcept { return current_state == GameState::InBattle; }

  [[nodiscard]] constexpr bool InMenu() const noexcept { return current_state == GameState::MainMenu; }

  static constexpr std::string_view GetName() noexcept { return "GameStateResource"; }
};

// ============================================================================
// Battle State Resource
// ============================================================================

struct BattleResource {
  helios::ecs::Entity player_entity{};
  helios::ecs::Entity enemy_entity{};
  int current_turn = 0;
  bool player_turn = true;
  bool battle_ended = false;
  bool player_won = false;
  helios::container::StaticString<128> battle_log;

  void SetLog(std::string_view message) noexcept { battle_log.Assign(message); }

  [[nodiscard]] std::string_view Log() const noexcept { return battle_log.View(); }

  void Clear() noexcept {
    player_entity = helios::ecs::Entity{};
    enemy_entity = helios::ecs::Entity{};
    current_turn = 0;
    player_turn = true;
    battle_ended = false;
    player_won = false;
    battle_log.Clear();
  }

  static constexpr std::string_view GetName() noexcept { return "BattleResource"; }
};

// ============================================================================
// Dialog State Resource
// ============================================================================

struct DialogResource {
  helios::ecs::Entity speaker_entity{};
  helios::container::StaticString<256> current_text;
  DialogOptions options{};
  bool waiting_for_input = false;
  bool dialog_complete = false;
  int dialog_stage = 0;

  void SetText(std::string_view text) noexcept { current_text.Assign(text); }

  [[nodiscard]] std::string_view Text() const noexcept { return current_text.View(); }

  void Clear() noexcept {
    speaker_entity = helios::ecs::Entity{};
    current_text.Clear();
    options = DialogOptions{};
    waiting_for_input = false;
    dialog_complete = false;
    dialog_stage = 0;
  }

  static constexpr std::string_view GetName() noexcept { return "DialogResource"; }
};

// ============================================================================
// Input State Resource
// ============================================================================

enum class InputCommand : uint8_t {
  None,
  Up,
  Down,
  Left,
  Right,
  Confirm,
  Cancel,
  Menu,
  Quit,
  Attack,
  Defend,
  Magic,
  Item,
  Flee,
  Number1,
  Number2,
  Number3,
  Number4,
};

struct InputResource {
  InputCommand current_command = InputCommand::None;
  bool input_consumed = false;
  char raw_input = '\0';

  void SetCommand(InputCommand cmd) noexcept {
    current_command = cmd;
    input_consumed = false;
  }

  [[nodiscard]] InputCommand ConsumeCommand() noexcept {
    if (!input_consumed) {
      input_consumed = true;
      return current_command;
    }
    return InputCommand::None;
  }

  void Clear() noexcept {
    current_command = InputCommand::None;
    input_consumed = false;
    raw_input = '\0';
  }

  static constexpr std::string_view GetName() noexcept { return "InputResource"; }
};

// ============================================================================
// Console Output Resource
// ============================================================================

struct ConsoleBuffer {
  static constexpr size_t kMaxLines = 20;
  static constexpr size_t kLineLength = 80;

  std::array<helios::container::StaticString<kLineLength>, kMaxLines> lines = {};
  size_t current_line = 0;
  bool needs_redraw = true;
  bool clear_on_next = false;

  void AddLine(std::string_view text) noexcept {
    if (current_line >= kMaxLines) {
      // Scroll up
      for (size_t i = 0; i < kMaxLines - 1; ++i) {
        lines[i] = lines[i + 1];
      }
      current_line = kMaxLines - 1;
    }

    lines[current_line].Assign(text);
    ++current_line;
    needs_redraw = true;
  }

  void Clear() noexcept {
    for (auto& line : lines) {
      line.Clear();
    }
    current_line = 0;
    needs_redraw = true;
  }

  [[nodiscard]] std::string_view GetLine(size_t index) const noexcept {
    if (index < kMaxLines) {
      return lines[index].View();
    }
    return "";
  }

  static constexpr std::string_view GetName() noexcept { return "ConsoleBuffer"; }
};

// ============================================================================
// Metrics Resource
// ============================================================================

struct MetricsResource {
  std::atomic<uint64_t> frame_count{0};
  std::atomic<uint64_t> total_systems_executed{0};
  float current_fps = 0.0F;
  float average_fps = 0.0F;
  float min_fps = 1000.0F;
  float max_fps = 0.0F;
  float frame_time_ms = 0.0F;
  float accumulated_time = 0.0F;
  int fps_sample_count = 0;
  float fps_sum = 0.0F;

  MetricsResource() noexcept = default;

  MetricsResource(const MetricsResource& other) noexcept
      : frame_count(other.frame_count.load(std::memory_order_relaxed)),
        total_systems_executed(other.total_systems_executed.load(std::memory_order_relaxed)),
        current_fps(other.current_fps),
        average_fps(other.average_fps),
        min_fps(other.min_fps),
        max_fps(other.max_fps),
        frame_time_ms(other.frame_time_ms),
        accumulated_time(other.accumulated_time),
        fps_sample_count(other.fps_sample_count),
        fps_sum(other.fps_sum) {}

  MetricsResource(MetricsResource&& other) noexcept
      : frame_count(other.frame_count.load(std::memory_order_relaxed)),
        total_systems_executed(other.total_systems_executed.load(std::memory_order_relaxed)),
        current_fps(other.current_fps),
        average_fps(other.average_fps),
        min_fps(other.min_fps),
        max_fps(other.max_fps),
        frame_time_ms(other.frame_time_ms),
        accumulated_time(other.accumulated_time),
        fps_sample_count(other.fps_sample_count),
        fps_sum(other.fps_sum) {}

  MetricsResource& operator=(const MetricsResource&) = delete;
  MetricsResource& operator=(MetricsResource&&) = delete;

  ~MetricsResource() noexcept = default;

  void UpdateFps(float delta_seconds) noexcept {
    if (delta_seconds > 0.0F) {
      current_fps = 1.0F / delta_seconds;
      frame_time_ms = delta_seconds * 1000.0F;

      fps_sum += current_fps;
      ++fps_sample_count;
      average_fps = fps_sum / static_cast<float>(fps_sample_count);

      if (current_fps < min_fps && current_fps > 0.0F) {
        min_fps = current_fps;
      }
      if (current_fps > max_fps) {
        max_fps = current_fps;
      }
    }

    frame_count.fetch_add(1, std::memory_order_relaxed);
  }

  void Reset() noexcept {
    frame_count.store(0, std::memory_order_relaxed);
    total_systems_executed.store(0, std::memory_order_relaxed);
    current_fps = 0.0F;
    average_fps = 0.0F;
    min_fps = 1000.0F;
    max_fps = 0.0F;
    frame_time_ms = 0.0F;
    accumulated_time = 0.0F;
    fps_sample_count = 0;
    fps_sum = 0.0F;
  }

  static constexpr std::string_view GetName() noexcept { return "MetricsResource"; }
};

// ============================================================================
// Location Resource
// ============================================================================

struct LocationResource {
  Location current_location{LocationType::Town, "Starting Town"};
  bool can_encounter_enemies = false;
  float encounter_chance = 0.0F;
  float time_since_last_encounter = 0.0F;

  void MoveTo(LocationType type, std::string_view name) noexcept {
    current_location = Location{type, name};

    switch (type) {
      case LocationType::Forest:
        can_encounter_enemies = true;
        encounter_chance = 0.3F;
        break;
      case LocationType::Dungeon:
        can_encounter_enemies = true;
        encounter_chance = 0.5F;
        break;
      default:
        can_encounter_enemies = false;
        encounter_chance = 0.0F;
        break;
    }

    time_since_last_encounter = 0.0F;
  }

  static constexpr std::string_view GetName() noexcept { return "LocationResource"; }
};

// ============================================================================
// Random Number Resource
// ============================================================================

struct RandomResource {
  uint32_t seed = 12345;

  [[nodiscard]] uint32_t Next() noexcept {
    // Simple xorshift32 algorithm
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    return seed;
  }

  [[nodiscard]] int Range(int min_val, int max_val) noexcept {
    if (min_val >= max_val) {
      return min_val;
    }
    return min_val + static_cast<int>(Next() % static_cast<uint32_t>(max_val - min_val + 1));
  }

  [[nodiscard]] float Normalized() noexcept { return static_cast<float>(Next()) / static_cast<float>(0xFFFFFFFF); }

  [[nodiscard]] bool Chance(float probability) noexcept { return Normalized() < probability; }

  static constexpr std::string_view GetName() noexcept { return "RandomResource"; }
};

}  // namespace rpg
