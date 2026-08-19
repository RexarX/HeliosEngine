#include <pch.hpp>

#include <helios/ecs/message/manager.hpp>

#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/message.hpp>

#include <cstddef>
#include <span>

namespace helios::ecs {

void MessageManager::Clear() noexcept {
  registered_messages_.ResetAll();
  current_messages_.ResetAll();
  previous_messages_.ResetAll();
  current_ids_.ResetAll();
  previous_ids_.ResetAll();
  message_counts_.ResetAll();
  async_messages_.Reset();
}

void MessageManager::ClearAllQueues() noexcept {
  current_messages_.ClearAll();
  previous_messages_.ClearAll();
  ClearAllIds();
  async_messages_.Clear();
}

void MessageManager::Update() {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::Update");

  for (const auto& [type_index, metadata] : registered_messages_) {
    if (!metadata.is_async &&
        metadata.clear_policy == MessageClearPolicy::kAutomatic) {
      previous_messages_.Clear(type_index);
      if (auto* ids = previous_ids_.TryGet(type_index)) {
        ids->clear();
      }
    }
  }

  previous_messages_.Merge(current_messages_);
  AgeIds();
  current_messages_.ClearAll();
}

void MessageManager::AssignIds(MessageTypeIndex type_index, size_type count) {
  if (count == 0) [[unlikely]] {
    return;
  }

  auto& next_id = message_counts_.Ensure(type_index);
  auto& ids = current_ids_.Ensure(type_index);
  ids.reserve(ids.size() + count);
  for (size_type i = 0; i < count; ++i) {
    ids.push_back({.value = next_id++, .type = type_index});
  }
}

void MessageManager::ClearIds(MessageTypeIndex type_index) noexcept {
  if (auto* ids = current_ids_.TryGet(type_index)) {
    ids->clear();
  }
  if (auto* ids = previous_ids_.TryGet(type_index)) {
    ids->clear();
  }
}

void MessageManager::ClearAllIds() noexcept {
  current_ids_.ClearAll();
  previous_ids_.ClearAll();
}

void MessageManager::AgeIds() {
  for (auto&& [type_index, curr_ids] : current_ids_) {
    auto& prev_ids = previous_ids_.Ensure(type_index);
    prev_ids.insert(prev_ids.end(), curr_ids.begin(), curr_ids.end());
    curr_ids.clear();
  }
}

void MessageManager::RemoveIds(MessageTypeIndex /*type_index*/,
                               MessageIdList& ids,
                               std::span<const size_type> sorted_indices) {
  if (sorted_indices.empty()) {
    return;
  }

  size_type idx = sorted_indices.size();
  while (idx > 0) {
    --idx;
    size_type range_end = sorted_indices[idx] + 1;
    size_type range_start = sorted_indices[idx];
    while (idx > 0 && sorted_indices[idx - 1] == range_start - 1) {
      --idx;
      range_start = sorted_indices[idx];
    }
    ids.erase(ids.begin() + static_cast<ptrdiff_t>(range_start),
              ids.begin() + static_cast<ptrdiff_t>(range_end));
  }
}

auto MessageManager::IdsFor(const MessageIdMap& map,
                            MessageTypeIndex type_index) const noexcept
    -> std::span<const AnyMessageId> {
  const auto* ids = map.TryGet(type_index);
  return ids != nullptr ? std::span{ids->data(), ids->size()}
                        : std::span<const AnyMessageId>{};
}

}  // namespace helios::ecs
