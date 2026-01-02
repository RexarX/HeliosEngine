#pragma once

#include <helios/core_pch.hpp>

#include "components.hpp"

#include <helios/core/container/static_string.hpp>
#include <helios/core/ecs/entity.hpp>

#include <cstdint>
#include <string_view>

namespace rpg {

// ============================================================================
// Battle Events
// ============================================================================

struct BattleStartEvent {
  helios::ecs::Entity player;
  helios::ecs::Entity enemy;
  EnemyType enemy_type = EnemyType::Goblin;

  static constexpr std::string_view GetName() noexcept { return "BattleStartEvent"; }
};

struct BattleEndEvent {
  helios::ecs::Entity winner;
  helios::ecs::Entity loser;
  bool player_won = false;
  int xp_gained = 0;
  int gold_gained = 0;

  static constexpr std::string_view GetName() noexcept { return "BattleEndEvent"; }
};

struct AttackEvent {
  helios::ecs::Entity attacker;
  helios::ecs::Entity target;
  int damage = 0;
  bool is_critical = false;
  bool missed = false;

  static constexpr std::string_view GetName() noexcept { return "AttackEvent"; }
};

struct DefendEvent {
  helios::ecs::Entity defender;
  float defense_bonus = 0.5F;

  static constexpr std::string_view GetName() noexcept { return "DefendEvent"; }
};

struct MagicEvent {
  helios::ecs::Entity caster;
  helios::ecs::Entity target;
  int damage = 0;
  int mana_cost = 0;
  helios::container::StaticString<32> spell_name;

  MagicEvent() noexcept = default;

  MagicEvent(helios::ecs::Entity cast, helios::ecs::Entity tgt, int dmg, int cost, std::string_view name) noexcept
      : caster(cast), target(tgt), damage(dmg), mana_cost(cost), spell_name(name) {}

  [[nodiscard]] std::string_view SpellName() const noexcept { return spell_name.View(); }

  static constexpr std::string_view GetName() noexcept { return "MagicEvent"; }
};

struct FleeEvent {
  helios::ecs::Entity fleeing_entity;
  bool success = false;

  static constexpr std::string_view GetName() noexcept { return "FleeEvent"; }
};

struct DamageEvent {
  helios::ecs::Entity target;
  int amount = 0;
  bool is_magic = false;

  static constexpr std::string_view GetName() noexcept { return "DamageEvent"; }
};

struct HealEvent {
  helios::ecs::Entity target;
  int amount = 0;

  static constexpr std::string_view GetName() noexcept { return "HealEvent"; }
};

struct DeathEvent {
  helios::ecs::Entity entity;
  bool is_player = false;

  static constexpr std::string_view GetName() noexcept { return "DeathEvent"; }
};

// ============================================================================
// Progression Events
// ============================================================================

struct LevelUpEvent {
  helios::ecs::Entity entity;
  int new_level = 1;
  int stat_points_gained = 0;

  static constexpr std::string_view GetName() noexcept { return "LevelUpEvent"; }
};

struct XpGainEvent {
  helios::ecs::Entity entity;
  int amount = 0;

  static constexpr std::string_view GetName() noexcept { return "XpGainEvent"; }
};

struct GoldGainEvent {
  helios::ecs::Entity entity;
  int amount = 0;

  static constexpr std::string_view GetName() noexcept { return "GoldGainEvent"; }
};

// ============================================================================
// UI Events
// ============================================================================

struct MenuSelectEvent {
  int selected_index = 0;
  helios::container::StaticString<32> menu_name;

  MenuSelectEvent() noexcept = default;

  MenuSelectEvent(int index, std::string_view name) noexcept : selected_index(index), menu_name(name) {}

  [[nodiscard]] std::string_view MenuName() const noexcept { return menu_name.View(); }

  static constexpr std::string_view GetName() noexcept { return "MenuSelectEvent"; }
};

struct DialogAdvanceEvent {
  helios::ecs::Entity speaker;
  int next_stage = 0;
  int selected_option = -1;

  static constexpr std::string_view GetName() noexcept { return "DialogAdvanceEvent"; }
};

struct DialogEndEvent {
  helios::ecs::Entity speaker;
  int final_option = -1;

  static constexpr std::string_view GetName() noexcept { return "DialogEndEvent"; }
};

// ============================================================================
// Game Flow Events
// ============================================================================

struct GameStartEvent {
  static constexpr std::string_view GetName() noexcept { return "GameStartEvent"; }
};

struct GameOverEvent {
  bool player_won = false;

  static constexpr std::string_view GetName() noexcept { return "GameOverEvent"; }
};

struct LocationChangeEvent {
  LocationType new_location = LocationType::Town;
  helios::container::StaticString<32> location_name;

  LocationChangeEvent() noexcept = default;

  LocationChangeEvent(LocationType type, std::string_view name) noexcept : new_location(type), location_name(name) {}

  [[nodiscard]] std::string_view LocationName() const noexcept { return location_name.View(); }

  static constexpr std::string_view GetName() noexcept { return "LocationChangeEvent"; }
};

struct EncounterEvent {
  EnemyType enemy_type = EnemyType::Goblin;

  static constexpr std::string_view GetName() noexcept { return "EncounterEvent"; }
};

// ============================================================================
// Console Events
// ============================================================================

struct PrintEvent {
  helios::container::StaticString<128> message;

  PrintEvent() noexcept = default;

  explicit PrintEvent(std::string_view msg) noexcept : message(msg) {}

  [[nodiscard]] std::string_view Message() const noexcept { return message.View(); }

  static constexpr std::string_view GetName() noexcept { return "PrintEvent"; }
};

struct ClearScreenEvent {
  static constexpr std::string_view GetName() noexcept { return "ClearScreenEvent"; }
};

}  // namespace rpg
