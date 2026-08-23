#include <pch.hpp>

#include <helios/ecs/message/consumed_registry.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace helios::ecs {

void ConsumedMessagesRegistry::MergeFrom(
    const ConsumedMessagesRegistry& other) {
  for (const auto& [type_index, other_indices] : other.Data()) {
    if (other_indices.empty()) {
      continue;
    }

    auto& our_ids = EnsureIds(type_index);
    if (our_ids.empty()) {
      our_ids.insert(our_ids.end(), other_indices.begin(), other_indices.end());
      continue;
    }

    // In-place merge: append other ids, then inplace_merge, then deduplicate.
    // This reuses existing capacity in our_ids, avoiding a separate allocation.
    const auto original_size = our_ids.size();
    our_ids.insert(our_ids.end(), other_indices.begin(), other_indices.end());
    std::ranges::inplace_merge(
        our_ids, our_ids.begin() + static_cast<ptrdiff_t>(original_size));
    const auto [first, last] = std::ranges::unique(our_ids);
    our_ids.erase(first, last);
  }
}

void ConsumedMessagesRegistry::MergeFrom(ConsumedMessagesRegistry&& other) {
  for (auto& [type_index, other_indices] : other.consumed_) {
    if (other_indices.empty()) {
      continue;
    }

    auto& our_ids = EnsureIds(type_index);
    if (our_ids.empty()) {
      our_ids = std::move(other_indices);
      continue;
    }

    const auto original_size = our_ids.size();
    our_ids.insert(our_ids.end(),
                   std::make_move_iterator(other_indices.begin()),
                   std::make_move_iterator(other_indices.end()));
    std::ranges::inplace_merge(
        our_ids, our_ids.begin() + static_cast<ptrdiff_t>(original_size));
    const auto [first, last] = std::ranges::unique(our_ids);
    our_ids.erase(first, last);
  }

  other.consumed_.Clear();
}

auto ConsumedMessagesRegistry::TotalConsumedCount() const noexcept
    -> size_type {
  size_type total = 0;
  for (const auto& [_, indices] : consumed_) {
    total += indices.size();
  }
  return total;
}

}  // namespace helios::ecs
