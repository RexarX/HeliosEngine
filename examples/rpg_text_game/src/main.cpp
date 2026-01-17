#include <helios/core/app/app.hpp>
#include <helios/core/app/runners.hpp>
#include <helios/core/app/schedules.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/ecs/system.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/timer.hpp>

#include "../include/components.hpp"
#include "../include/events.hpp"
#include "../include/modules.hpp"
#include "../include/resources.hpp"
#include "../include/systems.hpp"

#include <format>
#include <iostream>
#include <string_view>

using namespace helios::app;
using namespace helios::ecs;
using namespace rpg;

// ============================================================================
// Player Setup System (runs once at startup)
// ============================================================================

struct PlayerSetupSystem final : public System {
  static constexpr std::string_view GetName() noexcept { return "PlayerSetupSystem"; }
  static constexpr AccessPolicy GetAccessPolicy() noexcept { return {}; }

  void Update(SystemContext& ctx) override {
    // Create the player entity
    auto player = ctx.ReserveEntity();
    auto player_cmd = ctx.EntityCommands(player);

    player_cmd.AddComponents(Player{}, CharacterName{"Hero"}, Stats{12, 10, 8, 10}, Health{100, 100}, Mana{50, 50},
                             Experience{1, 0, 100}, Gold{50}, Location{LocationType::Town, "Starting Town"});

    HELIOS_INFO("Player entity created!");
  }
};

// ============================================================================
// Game Runner
// ============================================================================

AppExitCode GameRunner(App& app, int max_frames = 1000) {
  int frame = 0;

  HELIOS_INFO("=== RPG Text Game Starting ===");
  HELIOS_INFO("Running for {} frames maximum", max_frames);

  helios::Timer game_timer;
  while (frame < max_frames) {
    app.TickTime();
    app.Update();
    ++frame;

    // Check if game wants to quit
    const auto& world = std::as_const(app).GetMainWorld();
    if (world.HasResource<GameStateResource>()) {
      const auto& game_state = world.ReadResource<GameStateResource>();
      if (game_state.should_quit) {
        HELIOS_INFO("Game requested quit at frame {}", frame);
        break;
      }
    }

    // Print console buffer periodically for visibility
    if (frame % 100 == 0) {
      if (world.HasResource<MetricsResource>()) {
        const auto& metrics = world.ReadResource<MetricsResource>();
        HELIOS_INFO("Frame {}: FPS={:.1f}, Avg={:.1f}, Min={:.1f}, Max={:.1f}", frame, metrics.current_fps,
                    metrics.average_fps, metrics.min_fps, metrics.max_fps);
      }
    }
  }

  // Print final statistics
  const auto& world = std::as_const(app).GetMainWorld();
  if (world.HasResource<MetricsResource>()) {
    const auto& metrics = world.ReadResource<MetricsResource>();
    HELIOS_INFO("");
    HELIOS_INFO("=== Game Statistics ===");
    HELIOS_INFO("Total frames: {}", metrics.frame_count.load());
    HELIOS_INFO("Total time: {:.2f}s", metrics.accumulated_time);
    HELIOS_INFO("Average FPS: {:.1f}", metrics.average_fps);
    HELIOS_INFO("Min FPS: {:.1f}", metrics.min_fps);
    HELIOS_INFO("Max FPS: {:.1f}", metrics.max_fps);
  }

  if (world.HasResource<ConsoleBuffer>()) {
    const auto& console = world.ReadResource<ConsoleBuffer>();
    HELIOS_INFO("");
    HELIOS_INFO("=== Final Console Output ===");
    for (size_t i = 0; i < ConsoleBuffer::kMaxLines; ++i) {
      std::string_view line = console.GetLine(i);
      if (!line.empty()) {
        HELIOS_INFO("{}", line);
      }
    }
  }

  // Print player final stats
  HELIOS_INFO("");
  HELIOS_INFO("=== Player Final Stats ===");
  HELIOS_INFO("Entity count: {}", world.EntityCount());

  const double total_time = game_timer.ElapsedMilliSec();
  HELIOS_INFO("");
  HELIOS_INFO("=== RPG Text Game Complete ===");
  HELIOS_INFO("Total execution time: {:.2f}ms", total_time);

  return AppExitCode::kSuccess;
}

// ============================================================================
// Main Entry Point
// ============================================================================

int main() {
  HELIOS_INFO("Initializing RPG Text Game Example...");

  // Create the application
  App app;

  // Add all game modules
  app.AddModules<CoreGameModule, BattleModule, ExplorationModule, ProgressionModule, UIModule, CleanupModule>();

  // Add player setup system (runs once at startup)
  app.AddSystem<PlayerSetupSystem>(kStartup);

  // Verify modules are registered
  HELIOS_INFO("Modules registered:");
  HELIOS_INFO("  - CoreGameModule: {}", app.ContainsModule<CoreGameModule>());
  HELIOS_INFO("  - BattleModule: {}", app.ContainsModule<BattleModule>());
  HELIOS_INFO("  - ExplorationModule: {}", app.ContainsModule<ExplorationModule>());
  HELIOS_INFO("  - ProgressionModule: {}", app.ContainsModule<ProgressionModule>());
  HELIOS_INFO("  - UIModule: {}", app.ContainsModule<UIModule>());
  HELIOS_INFO("  - CleanupModule: {}", app.ContainsModule<CleanupModule>());

  // Set the game runner - run for 1000 frames or until game ends
  app.SetRunner([](App& running_app) { return GameRunner(running_app, 1000); });

  // Run the game
  HELIOS_INFO("Starting game...");
  AppExitCode result = app.Run();

  if (result == AppExitCode::kSuccess) {
    HELIOS_INFO("Game exited successfully!");
  } else {
    HELIOS_ERROR("Game exited with errors!");
  }

  return static_cast<int>(result);
}
