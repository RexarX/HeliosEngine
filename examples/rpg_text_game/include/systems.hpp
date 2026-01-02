#pragma once

#include <helios/core_pch.hpp>

#include "components.hpp"
#include "events.hpp"
#include "resources.hpp"

#include <helios/core/app/access_policy.hpp>
#include <helios/core/app/system_context.hpp>
#include <helios/core/app/time.hpp>
#include <helios/core/ecs/system.hpp>

#include <format>
#include <string_view>

namespace rpg {

// ============================================================================
// Metrics Systems
// ============================================================================

struct MetricsUpdateSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "MetricsUpdateSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy().ReadResources<helios::app::Time>().WriteResources<MetricsResource>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    const auto& time = ctx.ReadResource<helios::app::Time>();
    auto& metrics = ctx.WriteResource<MetricsResource>();

    metrics.UpdateFps(time.DeltaSeconds());
    metrics.accumulated_time += time.DeltaSeconds();
  }
};

// ============================================================================
// Input Systems
// ============================================================================

struct InputProcessingSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "InputProcessingSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy().WriteResources<InputResource>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& input = ctx.WriteResource<InputResource>();

    // Simulate input processing - in a real game this would poll actual input
    // For the demo, we'll let the game run automatically
    input.Clear();
  }
};

// ============================================================================
// Game State Systems
// ============================================================================

struct GameStateSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "GameStateSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy().WriteResources<GameStateResource, ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& game_state = ctx.WriteResource<GameStateResource>();
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    // Handle state transitions
    if (game_state.current_state == GameState::MainMenu) {
      // Auto-start game for demo purposes
      game_state.TransitionTo(GameState::Exploring);
      console.AddLine("=== Welcome to Text RPG Adventure ===");
      console.AddLine("Your journey begins...");
      console.AddLine("");
    }
  }
};

// ============================================================================
// Battle Systems
// ============================================================================

struct BattleInitSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "BattleInitSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy().WriteResources<BattleResource, GameStateResource, ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& battle = ctx.WriteResource<BattleResource>();
    auto& game_state = ctx.WriteResource<GameStateResource>();
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    auto events = ctx.ReadEvents<BattleStartEvent>();
    for (const auto& event : events) {
      battle.player_entity = event.player;
      battle.enemy_entity = event.enemy;
      battle.current_turn = 1;
      battle.player_turn = true;
      battle.battle_ended = false;

      game_state.TransitionTo(GameState::InBattle);

      console.AddLine("");
      console.AddLine("=== BATTLE START ===");
      console.AddLine(std::format("A wild {} appears!", GetEnemyName(event.enemy_type)));
    }
  }

private:
  static std::string_view GetEnemyName(EnemyType type) noexcept {
    switch (type) {
      case EnemyType::Goblin:
        return "Goblin";
      case EnemyType::Skeleton:
        return "Skeleton";
      case EnemyType::Orc:
        return "Orc";
      case EnemyType::Dragon:
        return "Dragon";
      case EnemyType::Slime:
        return "Slime";
      default:
        return "Monster";
    }
  }
};

struct BattleActionSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "BattleActionSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy()
        .Query<const Player&, const Stats&, Health&, Mana&>()
        .Query<const Enemy&, const Stats&, Health&>()
        .WriteResources<BattleResource, RandomResource, ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& battle = ctx.WriteResource<BattleResource>();
    auto& random = ctx.WriteResource<RandomResource>();
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    // Check if we're in a valid battle state
    if (battle.battle_ended || !battle.player_entity.Valid() || !battle.enemy_entity.Valid()) {
      return;
    }

    // Check if entities still exist
    if (!ctx.EntityExists(battle.player_entity) || !ctx.EntityExists(battle.enemy_entity)) {
      battle.battle_ended = true;
      return;
    }

    // Process battle turn
    if (battle.player_turn) {
      // Player attacks (auto-combat for demo)
      auto player_query = ctx.Query().With<Player>().Get<const Stats&>();
      auto enemy_query = ctx.Query().With<Enemy>().Get<Health&>();

      int player_attack = 0;
      player_query.ForEach([&player_attack](const Stats& stats) { player_attack = stats.AttackPower(); });

      int damage = std::max(1, player_attack + random.Range(-3, 3));

      enemy_query.ForEach([damage](Health& health) { health.TakeDamage(damage); });

      console.AddLine(std::format("You attack for {} damage!", damage));

      // Check if enemy died
      bool enemy_dead = false;
      enemy_query.ForEach([&enemy_dead](Health& health) { enemy_dead = health.Dead(); });

      if (enemy_dead) {
        battle.battle_ended = true;
        battle.player_won = true;
        console.AddLine("Enemy defeated!");
        ctx.EmitEvent(BattleEndEvent{battle.player_entity, battle.enemy_entity, true, 25, 10});
      } else {
        battle.player_turn = false;
      }
    } else {
      // Enemy attacks
      auto enemy_query = ctx.Query().With<Enemy>().Get<const Stats&>();
      auto player_query = ctx.Query().With<Player>().Get<Health&>();

      int enemy_attack = 0;
      enemy_query.ForEach([&enemy_attack](const Stats& stats) { enemy_attack = stats.AttackPower(); });

      int damage = std::max(1, enemy_attack + random.Range(-2, 2));

      player_query.ForEach([damage](Health& health) { health.TakeDamage(damage); });

      console.AddLine(std::format("Enemy attacks for {} damage!", damage));

      // Check if player died
      bool player_dead = false;
      player_query.ForEach([&player_dead](Health& health) { player_dead = health.Dead(); });

      if (player_dead) {
        battle.battle_ended = true;
        battle.player_won = false;
        console.AddLine("You have been defeated...");
        ctx.EmitEvent(BattleEndEvent{battle.enemy_entity, battle.player_entity, false, 0, 0});
      } else {
        battle.player_turn = true;
        ++battle.current_turn;
      }
    }
  }
};

struct BattleEndSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "BattleEndSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy()
        .Query<const Player&, Experience&, Gold&>()
        .WriteResources<BattleResource, GameStateResource, ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& battle = ctx.WriteResource<BattleResource>();
    auto& game_state = ctx.WriteResource<GameStateResource>();
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    auto events = ctx.ReadEvents<BattleEndEvent>();
    for (const auto& event : events) {
      if (event.player_won) {
        console.AddLine("=== VICTORY ===");
        console.AddLine(std::format("Gained {} XP and {} Gold!", event.xp_gained, event.gold_gained));

        ctx.Query().With<Player>().Get<Experience&, Gold&>().ForEach([&event](Experience& exp, Gold& gold) {
          exp.AddXp(event.xp_gained);
          gold.Add(event.gold_gained);
        });

        game_state.TransitionTo(GameState::Exploring);
      } else {
        console.AddLine("=== GAME OVER ===");
        game_state.TransitionTo(GameState::GameOver);
      }

      console.AddLine("");
      battle.Clear();
    }
  }
};

// ============================================================================
// Exploration Systems
// ============================================================================

struct ExplorationSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "ExplorationSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy()
        .ReadResources<helios::app::Time>()
        .WriteResources<GameStateResource, LocationResource, RandomResource, ConsoleBuffer, MetricsResource>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& game_state = ctx.WriteResource<GameStateResource>();
    auto& location = ctx.WriteResource<LocationResource>();
    auto& random = ctx.WriteResource<RandomResource>();
    auto& console = ctx.WriteResource<ConsoleBuffer>();
    auto& metrics = ctx.WriteResource<MetricsResource>();
    const auto& time = ctx.ReadResource<helios::app::Time>();

    if (game_state.current_state != GameState::Exploring) {
      return;
    }

    // Update exploration time
    location.time_since_last_encounter += time.DeltaSeconds();

    // Move to forest for encounters after starting
    if (location.current_location.type == LocationType::Town && metrics.frame_count.load() > 10) {
      location.MoveTo(LocationType::Forest, "Dark Forest");
      console.AddLine("You venture into the Dark Forest...");
    }

    // Check for random encounters
    if (location.can_encounter_enemies && location.time_since_last_encounter > 2.0F) {
      if (random.Chance(location.encounter_chance * time.DeltaSeconds())) {
        location.time_since_last_encounter = 0.0F;

        // Generate random enemy
        auto enemy_type = static_cast<EnemyType>(random.Range(0, 4));
        ctx.EmitEvent(EncounterEvent{enemy_type});
      }
    }
  }
};

struct EncounterSpawnSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "EncounterSpawnSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return helios::app::AccessPolicy().Query<const Player&>(); }

  void Update(helios::app::SystemContext& ctx) override {
    auto events = ctx.ReadEvents<EncounterEvent>();

    for (const auto& event : events) {
      // Find player entity
      helios::ecs::Entity player_entity{};
      ctx.Query().With<Player>().Get<const Player&>().ForEachWithEntity(
          [&player_entity](helios::ecs::Entity entity, const Player&) { player_entity = entity; });

      if (!ctx.EntityExists(player_entity)) {
        continue;
      }

      // Create enemy entity
      auto enemy = ctx.ReserveEntity();
      auto enemy_cmd = ctx.EntityCommands(enemy);

      Stats enemy_stats;
      Health enemy_health;
      EnemyInfo enemy_info{event.enemy_type, 0, 0};
      CharacterName enemy_name;

      switch (event.enemy_type) {
        case EnemyType::Goblin:
          enemy_stats = Stats{8, 12, 5, 6};
          enemy_health = Health{30, 30};
          enemy_info = EnemyInfo{EnemyType::Goblin, 15, 8};
          enemy_name = CharacterName{"Goblin"};
          break;
        case EnemyType::Skeleton:
          enemy_stats = Stats{10, 8, 3, 8};
          enemy_health = Health{40, 40};
          enemy_info = EnemyInfo{EnemyType::Skeleton, 20, 12};
          enemy_name = CharacterName{"Skeleton"};
          break;
        case EnemyType::Orc:
          enemy_stats = Stats{15, 6, 2, 12};
          enemy_health = Health{60, 60};
          enemy_info = EnemyInfo{EnemyType::Orc, 35, 20};
          enemy_name = CharacterName{"Orc"};
          break;
        case EnemyType::Dragon:
          enemy_stats = Stats{20, 10, 15, 18};
          enemy_health = Health{150, 150};
          enemy_info = EnemyInfo{EnemyType::Dragon, 100, 100};
          enemy_name = CharacterName{"Dragon"};
          break;
        case EnemyType::Slime:
        default:
          enemy_stats = Stats{5, 5, 2, 4};
          enemy_health = Health{20, 20};
          enemy_info = EnemyInfo{EnemyType::Slime, 10, 5};
          enemy_name = CharacterName{"Slime"};
          break;
      }

      enemy_cmd.AddComponents(Enemy{}, std::move(enemy_stats), std::move(enemy_health), std::move(enemy_info),
                              std::move(enemy_name));

      ctx.EmitEvent(BattleStartEvent{player_entity, enemy, event.enemy_type});
    }
  }
};

// ============================================================================
// Level Up System
// ============================================================================

struct LevelUpSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "LevelUpSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy()
        .Query<const Player&, Experience&, Stats&, Health&>()
        .WriteResources<ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    ctx.Query().With<Player>().Get<Experience&, Stats&, Health&>().ForEach(
        [&console](Experience& exp, Stats& stats, Health& health) {
          while (exp.CanLevelUp()) {
            exp.LevelUp();

            // Boost stats on level up
            stats.strength += 2;
            stats.dexterity += 1;
            stats.intelligence += 1;
            stats.vitality += 2;

            // Increase max HP and heal fully
            health.max_hp += 10;
            health.current_hp = health.max_hp;

            console.AddLine("");
            console.AddLine(std::format("*** LEVEL UP! You are now level {}! ***", exp.level));
            console.AddLine("Stats increased! HP restored!");
          }
        });
  }
};

// ============================================================================
// Console Output System
// ============================================================================

struct ConsoleRenderSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "ConsoleRenderSystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy().WriteResources<ConsoleBuffer>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& console = ctx.WriteResource<ConsoleBuffer>();

    if (console.needs_redraw) {
      // In a real implementation, this would render to terminal
      // For now, we just mark as rendered
      console.needs_redraw = false;
    }
  }
};

// ============================================================================
// Status Display System
// ============================================================================

struct StatusDisplaySystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "StatusDisplaySystem"; }
  static constexpr auto GetAccessPolicy() noexcept {
    return helios::app::AccessPolicy()
        .Query<const Player&, const Health&, const Mana&, const Experience&, const Gold&>()
        .ReadResources<MetricsResource, helios::app::Time>()
        .WriteResources<ConsoleBuffer, GameStateResource>();
  }

  void Update(helios::app::SystemContext& ctx) override {
    auto& console = ctx.WriteResource<ConsoleBuffer>();
    auto& game_state = ctx.WriteResource<GameStateResource>();
    const auto& metrics = ctx.ReadResource<MetricsResource>();
    const auto& time = ctx.ReadResource<helios::app::Time>();

    // Only update status every second
    static float time_since_status = 0.0F;
    time_since_status += time.DeltaSeconds();

    if (time_since_status < 1.0F) {
      return;
    }
    time_since_status = 0.0F;

    // Display metrics
    console.AddLine(std::format("[FPS: {:.1f} | Frame: {} | Time: {:.1f}s]", metrics.current_fps,
                                metrics.frame_count.load(), metrics.accumulated_time));

    // Display player status if exploring
    if (game_state.current_state == GameState::Exploring) {
      ctx.Query().With<Player>().Get<const Health&, const Mana&, const Experience&, const Gold&>().ForEach(
          [&console](const Health& hp, const Mana& mp, const Experience& exp, const Gold& gold) {
            console.AddLine(std::format("HP: {}/{} | MP: {}/{} | Lv: {} | Gold: {}", hp.current_hp, hp.max_hp,
                                        mp.current_mp, mp.max_mp, exp.level, gold.amount));
          });
    }

    // Check for game end condition (demo ends after some time)
    if (metrics.accumulated_time > 10.0F && game_state.current_state == GameState::Exploring) {
      console.AddLine("");
      console.AddLine("=== DEMO COMPLETE ===");
      console.AddLine("Thank you for playing!");
      game_state.should_quit = true;
    }
  }
};

// ============================================================================
// Cleanup System
// ============================================================================

struct DeadEntityCleanupSystem final : public helios::ecs::System {
  static constexpr std::string_view GetName() noexcept { return "DeadEntityCleanupSystem"; }
  static constexpr auto GetAccessPolicy() noexcept { return helios::app::AccessPolicy().Query<const Health&>(); }

  void Update(helios::app::SystemContext& ctx) override {
    // Query for entities with Health component that are dead
    ctx.Query().Get<const Health&>().ForEachWithEntity([&ctx](helios::ecs::Entity entity, const Health& health) {
      if (health.Dead()) {
        ctx.Commands().Destroy(entity);
      }
    });
  }
};

}  // namespace rpg
