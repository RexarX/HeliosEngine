#pragma once

#include <atomic>
#include <cstdint>

namespace helios::mem::details {

/// @brief Packed Treiber-stack head: low 48 bits pointer, high 16 bits ABA tag.
struct TaggedFreeHead {
  static constexpr uint32_t kPtrBits = 48;
  static constexpr uint32_t kTagBits = 64 - kPtrBits;
  static constexpr uintptr_t kPtrMask = (uintptr_t{1} << kPtrBits) - 1;
  static constexpr uintptr_t kTagMask = ~kPtrMask;

  static_assert(std::atomic<uintptr_t>::is_always_lock_free,
                "Tagged Treiber stack requires lock-free uintptr_t atomics");

  [[nodiscard]] static uintptr_t Pack(void* ptr, uint16_t tag) noexcept {
    return (reinterpret_cast<uintptr_t>(ptr) & kPtrMask) |
           (static_cast<uintptr_t>(tag) << kPtrBits);
  }

  [[nodiscard]] static void* Ptr(uintptr_t packed) noexcept {
    return reinterpret_cast<void*>(packed & kPtrMask);
  }

  [[nodiscard]] static uint16_t Tag(uintptr_t packed) noexcept {
    return static_cast<uint16_t>((packed & kTagMask) >> kPtrBits);
  }

  [[nodiscard]] static uintptr_t NextTag(uintptr_t packed) noexcept {
    return Pack(Ptr(packed), static_cast<uint16_t>(Tag(packed) + 1));
  }
};

inline void PushBlock(std::atomic<uintptr_t>& head, void* block) noexcept {
  uintptr_t observed = head.load(std::memory_order_acquire);
  for (;;) {
    *static_cast<void**>(block) = TaggedFreeHead::Ptr(observed);
    const uintptr_t desired = TaggedFreeHead::Pack(
        block, static_cast<uint16_t>(TaggedFreeHead::Tag(observed) + 1));
    if (head.compare_exchange_weak(observed, desired, std::memory_order_release,
                                   std::memory_order_acquire)) {
      return;
    }
  }
}

[[nodiscard]] inline void* PopBlock(std::atomic<uintptr_t>& head) noexcept {
  uintptr_t observed = head.load(std::memory_order_acquire);
  for (;;) {
    void* const block = TaggedFreeHead::Ptr(observed);
    if (block == nullptr) {
      return nullptr;
    }
    void* const next = *static_cast<void**>(block);
    const uintptr_t desired = TaggedFreeHead::Pack(
        next, static_cast<uint16_t>(TaggedFreeHead::Tag(observed) + 1));
    if (head.compare_exchange_weak(observed, desired, std::memory_order_release,
                                   std::memory_order_acquire)) {
      return block;
    }
  }
}

inline void StoreEmptyHead(std::atomic<uintptr_t>& head) noexcept {
  head.store(0, std::memory_order_release);
}

}  // namespace helios::mem::details
