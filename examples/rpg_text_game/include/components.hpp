#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/container/static_string.hpp>

#include <array>
#include <cstdint>

namespace rpg {

// ============================================================================
// Character Components
// ============================================================================

struct Stats {
  int strength = 10;
  int dexterity = 10;
  int intelligence = 10;
  int vitality = 10;

  [[nodiscard]] constexpr int AttackPower() const noexcept { return strength * 2; }
  [[nodiscard]] constexpr int Defense() const noexcept { return vitality; }
  [[nodiscard]] constexpr int Speed() const noexcept { return dexterity; }
  [[nodiscard]] constexpr int MagicPower() const noexcept { return intelligence * 2; }
};

struct Health {
  int max_hp = 100;
  int current_hp = 100;

  constexpr void TakeDamage(int damage) noexcept { current_hp = std::max(0, current_hp - damage); }

  constexpr void Heal(int amount) noexcept { current_hp = std::min(max_hp, current_hp + amount); }

  [[nodiscard]] constexpr bool Dead() const noexcept { return current_hp <= 0; }

  [[nodiscard]] constexpr float Percentage() const noexcept {
    return max_hp > 0 ? static_cast<float>(current_hp) / static_cast<float>(max_hp) : 0.0F;
  }
};

struct Mana {
  int max_mp = 50;
  int current_mp = 50;

  [[nodiscard]] constexpr bool CanCast(int cost) const noexcept { return current_mp >= cost; }

  constexpr void Spend(int amount) noexcept { current_mp = std::max(0, current_mp - amount); }

  constexpr void Restore(int amount) noexcept { current_mp = std::min(max_mp, current_mp + amount); }
};

struct Experience {
  int level = 1;
  int current_xp = 0;
  int xp_to_next_level = 100;

  [[nodiscard]] constexpr bool CanLevelUp() const noexcept { return current_xp >= xp_to_next_level; }

  constexpr void AddXp(int amount) noexcept { current_xp += amount; }

  constexpr void LevelUp() noexcept {
    if (CanLevelUp()) {
      current_xp -= xp_to_next_level;
      ++level;
      xp_to_next_level = level * 100;
    }
  }
};

struct CharacterName {
  helios::container::StaticString<32> name;

  CharacterName() noexcept = default;

  explicit CharacterName(std::string_view str) noexcept : name(str) {}

  [[nodiscard]] std::string_view View() const noexcept { return name.View(); }
};

struct Gold {
  int amount = 0;

  constexpr void Add(int value) noexcept { amount += value; }

  [[nodiscard]] constexpr bool CanSpend(int value) const noexcept { return amount >= value; }

  constexpr bool Spend(int value) noexcept {
    if (CanSpend(value)) {
      amount -= value;
      return true;
    }
    return false;
  }
};

// ============================================================================
// Tag Components
// ============================================================================

struct Player {};
struct Enemy {};
struct Npc {};
struct InBattle {};
struct Dead {};
struct Fleeing {};

// ============================================================================
// Enemy Types
// ============================================================================

enum class EnemyType : uint8_t {
  Goblin,
  Skeleton,
  Orc,
  Dragon,
  Slime,
};

struct EnemyInfo {
  EnemyType type = EnemyType::Goblin;
  int xp_reward = 10;
  int gold_reward = 5;
};

// ============================================================================
// Battle Components
// ============================================================================

enum class BattleAction : uint8_t {
  None,
  Attack,
  Defend,
  Magic,
  UseItem,
  Flee,
};

struct BattleState {
  BattleAction pending_action = BattleAction::None;
  int selected_target = 0;
  bool is_defending = false;
  float defense_multiplier = 1.0F;
};

struct TurnOrder {
  int initiative = 0;
  bool has_acted_this_turn = false;
};

// ============================================================================
// Dialog Components
// ============================================================================

struct DialogSpeaker {
  helios::container::StaticString<32> name;

  DialogSpeaker() noexcept = default;

  explicit DialogSpeaker(std::string_view str) noexcept : name(str) {}

  [[nodiscard]] std::string_view View() const noexcept { return name.View(); }
};

struct DialogOptions {
  static constexpr size_t kMaxOptions = 4;
  std::array<helios::container::StaticString<64>, kMaxOptions> options = {};
  int option_count = 0;
  int selected_option = 0;

  void AddOption(std::string_view text) noexcept {
    if (option_count < static_cast<int>(kMaxOptions)) {
      options[static_cast<size_t>(option_count)].Assign(text);
      ++option_count;
    }
  }

  [[nodiscard]] std::string_view GetOption(int index) const noexcept {
    if (index >= 0 && index < option_count) {
      return options[static_cast<size_t>(index)].View();
    }
    return "";
  }
};

// ============================================================================
// Location Components
// ============================================================================

enum class LocationType : uint8_t {
  Town,
  Forest,
  Dungeon,
  Shop,
  Inn,
};

struct Location {
  LocationType type = LocationType::Town;
  helios::container::StaticString<32> name;

  Location() noexcept = default;

  Location(LocationType loc_type, std::string_view loc_name) noexcept : type(loc_type), name(loc_name) {}

  [[nodiscard]] std::string_view Name() const noexcept { return name.View(); }
};

}  // namespace rpg
