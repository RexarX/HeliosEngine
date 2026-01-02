#pragma once

#include <helios/core_pch.hpp>

#include "components.hpp"
#include "events.hpp"
#include "resources.hpp"
#include "systems.hpp"

#include <helios/core/app/app.hpp>
#include <helios/core/app/module.hpp>
#include <helios/core/app/schedules.hpp>

#include <string_view>

namespace rpg {

// ============================================================================
// Core Game Module
// ============================================================================

struct CoreGameModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "CoreGameModule"; }

  void Build(helios::app::App& app) override {
    // Register resources
    app.InsertResource(GameStateResource{})
        .InsertResource(InputResource{})
        .InsertResource(ConsoleBuffer{})
        .InsertResource(MetricsResource{})
        .InsertResource(RandomResource{})
        .InsertResource(LocationResource{});

    // Register events
    app.AddEvent<GameStartEvent>()
        .AddEvent<GameOverEvent>()
        .AddEvent<PrintEvent>()
        .AddEvent<ClearScreenEvent>()
        .AddEvent<MenuSelectEvent>();

    // Add core systems
    app.AddSystem<MetricsUpdateSystem>(helios::app::kFirst);
    app.AddSystem<InputProcessingSystem>(helios::app::kPreUpdate);
    app.AddSystem<GameStateSystem>(helios::app::kUpdate);
    app.AddSystem<ConsoleRenderSystem>(helios::app::kLast);
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

// ============================================================================
// Battle Module
// ============================================================================

struct BattleModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "BattleModule"; }

  void Build(helios::app::App& app) override {
    // Register battle resources
    app.InsertResource(BattleResource{});

    // Register battle events
    app.AddEvent<BattleStartEvent>()
        .AddEvent<BattleEndEvent>()
        .AddEvent<AttackEvent>()
        .AddEvent<DefendEvent>()
        .AddEvent<MagicEvent>()
        .AddEvent<FleeEvent>()
        .AddEvent<DamageEvent>()
        .AddEvent<HealEvent>()
        .AddEvent<DeathEvent>();

    // Add battle systems with proper ordering
    app.AddSystemBuilder<BattleInitSystem>(helios::app::kUpdate).After<GameStateSystem>();

    app.AddSystemBuilder<BattleActionSystem>(helios::app::kUpdate).After<BattleInitSystem>();

    app.AddSystemBuilder<BattleEndSystem>(helios::app::kPostUpdate);
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

// ============================================================================
// Exploration Module
// ============================================================================

struct ExplorationModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "ExplorationModule"; }

  void Build(helios::app::App& app) override {
    // Register exploration events
    app.AddEvent<LocationChangeEvent>().AddEvent<EncounterEvent>();

    // Add exploration systems
    app.AddSystemBuilder<ExplorationSystem>(helios::app::kUpdate).After<GameStateSystem>();

    app.AddSystemBuilder<EncounterSpawnSystem>(helios::app::kUpdate).After<ExplorationSystem>();
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

// ============================================================================
// Progression Module
// ============================================================================

struct ProgressionModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "ProgressionModule"; }

  void Build(helios::app::App& app) override {
    // Register progression events
    app.AddEvent<LevelUpEvent>().AddEvent<XpGainEvent>().AddEvent<GoldGainEvent>();

    // Add progression systems
    app.AddSystem<LevelUpSystem>(helios::app::kPostUpdate);
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

// ============================================================================
// UI Module
// ============================================================================

struct UIModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "UIModule"; }

  void Build(helios::app::App& app) override {
    // Register UI events
    app.AddEvent<DialogAdvanceEvent>().AddEvent<DialogEndEvent>();

    // Register dialog resource
    app.InsertResource(DialogResource{});

    // Add UI systems
    app.AddSystem<StatusDisplaySystem>(helios::app::kLast);
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

// ============================================================================
// Cleanup Module
// ============================================================================

struct CleanupModule final : public helios::app::Module {
  static constexpr std::string_view GetName() noexcept { return "CleanupModule"; }

  void Build(helios::app::App& app) override {
    // Add cleanup systems that run at the end of each frame
    app.AddSystem<DeadEntityCleanupSystem>(helios::app::kLast);
  }

  void Destroy(helios::app::App& /*app*/) override {}
};

}  // namespace rpg
