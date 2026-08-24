#pragma once

#include <helios/assert.hpp>
#include <helios/ecs/schedule/stage.hpp>

#include <algorithm>
#include <span>
#include <vector>

namespace helios::app {

/// @brief Ordered list of stages to run for one frame pass.
class FrameOrder {
public:
  constexpr FrameOrder() noexcept = default;

  /**
   * @brief Inserts `stage` immediately after `after`.
   * @warning Triggers assertion if `after` is not in the order, or if `stage`
   * is already present.
   * @param after Stage type index to insert after
   * @param stage Stage type index to insert
   */
  constexpr void InsertAfter(ecs::StageTypeIndex after,
                             ecs::StageTypeIndex stage);

  /**
   * @brief Inserts `stage` immediately after `after`.
   * @warning Triggers assertion if `after` is not in the order, or if `stage`
   * is already present.
   * @tparam After Stage type to insert after
   * @tparam Stage Stage type to insert
   * @param after Optional instance of `After` to deduce type
   * @param stage Optional instance of `Stage` to deduce type
   */
  template <ecs::StageTrait After, ecs::StageTrait Stage>
  constexpr void InsertAfter(const After& after = {}, const Stage& stage = {}) {
    InsertAfter(ecs::StageTypeIndex::From(after),
                ecs::StageTypeIndex::From(stage));
  }

  /**
   * @brief Inserts `stage` immediately before `before`.
   * @warning Triggers assertion if `before` is not in the order, or if `stage`
   * is already present.
   * @param before Stage type index to insert before
   * @param stage Stage type index to insert
   */
  constexpr void InsertBefore(ecs::StageTypeIndex before,
                              ecs::StageTypeIndex stage);

  /**
   * @brief Inserts `stage` immediately before `before`.
   * @warning Triggers assertion if `before` is not in the order, or if `stage`
   * is already present.
   * @tparam Before Stage type to insert before
   * @tparam Stage Stage type to insert
   * @param before Optional instance of `Before` to deduce type
   * @param stage Optional instance of `Stage` to deduce type
   */
  template <ecs::StageTrait Before, ecs::StageTrait Stage>
  constexpr void InsertBefore(const Before& before = {},
                              const Stage& stage = {}) {
    InsertBefore(ecs::StageTypeIndex::From(before),
                 ecs::StageTypeIndex::From(stage));
  }

  /**
   * @brief Appends `stage` if not already present.
   * @param stage Stage type index to append
   * @return True if inserted
   */
  constexpr bool TryPushBack(ecs::StageTypeIndex stage);

  /**
   * @brief Appends `stage` if not already present.
   * @tparam Stage Stage type to append
   * @param stage Optional instance of `Stage` to deduce type
   * @return True if inserted
   */
  template <ecs::StageTrait Stage>
  constexpr bool TryPushBack(const Stage& stage = {}) {
    return TryPushBack(ecs::StageTypeIndex::From(stage));
  }

  /**
   * @brief Prepends `stage` if not already present.
   * @param stage Stage type index to prepend
   * @return True if inserted
   */
  constexpr bool TryPushFront(ecs::StageTypeIndex stage);

  /**
   * @brief Prepends `stage` if not already present.
   * @tparam Stage Stage type to prepend
   * @param stage Optional instance of `Stage` to deduce type
   * @return True if inserted
   */
  template <ecs::StageTrait Stage>
  constexpr bool TryPushFront(const Stage& stage = {}) {
    return TryPushFront(ecs::StageTypeIndex::From(stage));
  }

  /**
   * @brief Checks if `stage` is present in the order.
   * @param stage Stage type index to check
   * @return True if present
   */
  [[nodiscard]] constexpr bool Contains(
      ecs::StageTypeIndex stage) const noexcept {
    return std::ranges::any_of(
        labels_, [stage](ecs::StageTypeIndex label) { return label == stage; });
  }

  /**
   * @brief Checks if `stage` is present in the order.
   * @tparam Stage Stage type to check
   * @param stage Optional instance of `Stage` to deduce type
   * @return True if present
   */
  template <ecs::StageTrait Stage>
  [[nodiscard]] constexpr bool Contains(
      const Stage& stage = {}) const noexcept {
    return Contains(ecs::StageTypeIndex::From(stage));
  }

  /**
   * @brief Returns the ordered list of stage type indices.
   * @return Span of stage type indices
   */
  [[nodiscard]] constexpr auto Labels() const noexcept
      -> std::span<const ecs::StageTypeIndex> {
    return labels_;
  }

private:
  std::vector<ecs::StageTypeIndex> labels_;
};

/// @brief Stages run by `App::Update()` each frame.
struct MainFrameOrder final : public FrameOrder {};

/// @brief Stages run by nested frame pumps (e.g. OS modal loops).
struct FramePumpOrder final : public FrameOrder {};

constexpr void FrameOrder::InsertAfter(ecs::StageTypeIndex after,
                                       ecs::StageTypeIndex stage) {
  HELIOS_ASSERT(!Contains(stage), "Stage already present in frame order!");
  const auto it = std::ranges::find_if(
      labels_, [after](ecs::StageTypeIndex label) { return label == after; });
  HELIOS_ASSERT(it != labels_.end(),
                "Anchor stage not found in frame order for InsertAfter!");
  labels_.insert(it + 1, stage);
}

constexpr void FrameOrder::InsertBefore(ecs::StageTypeIndex before,
                                        ecs::StageTypeIndex stage) {
  HELIOS_ASSERT(!Contains(stage), "Stage already present in frame order!");
  const auto it = std::ranges::find_if(
      labels_, [before](ecs::StageTypeIndex label) { return label == before; });
  HELIOS_ASSERT(it != labels_.end(),
                "Anchor stage not found in frame order for InsertBefore!");
  labels_.insert(it, stage);
}

constexpr bool FrameOrder::TryPushBack(ecs::StageTypeIndex stage) {
  if (Contains(stage)) {
    return false;
  }

  labels_.push_back(stage);
  return true;
}

constexpr bool FrameOrder::TryPushFront(ecs::StageTypeIndex stage) {
  if (Contains(stage)) {
    return false;
  }

  labels_.insert(labels_.begin(), stage);
  return true;
}

}  // namespace helios::app
