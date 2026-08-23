#include <pch.hpp>

#include <helios/ecs/message/manager.hpp>

#include <helios/ecs/details/profile.hpp>
#include <helios/ecs/message/consumed_registry.hpp>
#include <helios/ecs/message/id.hpp>
#include <helios/ecs/message/message.hpp>
#include <helios/ecs/message/queue.hpp>
#include <helios/memory/temporary_storage.hpp>

#include <algorithm>
#include <cstddef>
#include <memory_resource>
#include <span>
#include <vector>

namespace helios::ecs {

MessageManager::MessageManager(std::pmr::memory_resource* resource)
    : resource_(resource),
      registered_messages_(resource),
      current_messages_(resource),
      previous_messages_(resource),
      current_ids_(resource),
      previous_ids_(resource),
      message_counts_(resource) {}

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

void MessageManager::Update(const ConsumedMessagesRegistry& consumed_registry) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::Update");

  if (!consumed_registry.Empty()) {
    ApplyConsumed(consumed_registry);
  }

  previous_messages_.Merge(current_messages_);
  AgeIds();
  current_messages_.ClearAll();
}

void MessageManager::ApplyConsumed(
    const ConsumedMessagesRegistry& merged_consumed) {
  HELIOS_ECS_PROFILE_SCOPE_N("helios::ecs::MessageManager::ApplyConsumed");
  HELIOS_ECS_PROFILE_ZONE_VALUE(merged_consumed.TotalConsumedCount());

  for (const auto& [type_index, consumed_ids] : merged_consumed.Data()) {
    if (consumed_ids.empty()) {
      continue;
    }

    const auto* metadata = registered_messages_.TryGet(type_index);
    if (metadata == nullptr || metadata->is_async) {
      continue;
    }

    auto* prev_ids = previous_ids_.TryGet(type_index);
    auto* curr_ids = current_ids_.TryGet(type_index);
    if ((prev_ids == nullptr || prev_ids->empty()) &&
        (curr_ids == nullptr || curr_ids->empty())) {
      continue;
    }

    std::pmr::vector<size_type> prev_indices{&mem::GetTemporaryStorage()};
    std::pmr::vector<size_type> curr_indices{&mem::GetTemporaryStorage()};
    prev_indices.reserve(consumed_ids.size());
    curr_indices.reserve(consumed_ids.size());

    for (const AnyMessageId id : consumed_ids) {
      if (prev_ids != nullptr) {
        const auto it = std::ranges::lower_bound(*prev_ids, id);
        if (it != prev_ids->end() && *it == id) {
          prev_indices.push_back(
              static_cast<size_type>(it - prev_ids->begin()));
          continue;
        }
      }
      if (curr_ids != nullptr) {
        const auto it = std::ranges::lower_bound(*curr_ids, id);
        if (it != curr_ids->end() && *it == id) {
          curr_indices.push_back(
              static_cast<size_type>(it - curr_ids->begin()));
        }
      }
    }

    if (!prev_indices.empty()) {
      previous_messages_.RemoveIndices(type_index, prev_indices);
      RemoveIds(type_index, *prev_ids, prev_indices);
    }
    if (!curr_indices.empty()) {
      current_messages_.RemoveIndices(type_index, curr_indices);
      RemoveIds(type_index, *curr_ids, curr_indices);
    }
  }
}

void MessageManager::MergeLocalMessages(const MessageQueue& local) {
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (metadata.is_async) {
      continue;
    }
    AssignIds(type_index, local.MessageCount(type_index));
  }
  current_messages_.Merge(local);
}

void MessageManager::MergeLocalMessages(MessageQueue&& local) {
  for (const auto& [type_index, metadata] : registered_messages_) {
    if (metadata.is_async) {
      continue;
    }
    AssignIds(type_index, local.MessageCount(type_index));
  }
  current_messages_.Merge(std::move(local));
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
  for (auto& [type_index, curr_ids] : current_ids_) {
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
