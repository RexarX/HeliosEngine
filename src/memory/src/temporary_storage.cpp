#include <pch.hpp>

#include <helios/memory/temporary_storage.hpp>

#include <helios/assert.hpp>
#include <helios/memory/details/profile.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory_resource>
#include <optional>

namespace helios::mem {

namespace {

#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
/// @brief Sits between a `monotonic_buffer_resource` and its true upstream,
/// profiling only the allocations the monotonic resource makes when it grow.
class TemporaryStorageUpstream final : public std::pmr::memory_resource {
public:
  explicit TemporaryStorageUpstream(
      std::pmr::memory_resource* upstream) noexcept
      : upstream_(upstream) {}

private:
  void* do_allocate(size_t bytes, size_t alignment) override {
    void* const ptr = upstream_->allocate(bytes, alignment);
    HELIOS_MEMORY_PROFILE_ALLOC(ptr, bytes, "TemporaryStorage");
    return ptr;
  }

  void do_deallocate(void* ptr, size_t bytes, size_t alignment) override {
    HELIOS_MEMORY_PROFILE_FREE(ptr, "TemporaryStorage");
    upstream_->deallocate(ptr, bytes, alignment);
  }

  [[nodiscard]] bool do_is_equal(
      const std::pmr::memory_resource& other) const noexcept override {
    return this == &other;
  }

  std::pmr::memory_resource* upstream_ = std::pmr::new_delete_resource();
};
#endif

struct Block {
  alignas(std::max_align_t)
      std::array<std::byte, TemporaryStorage::kInitialBlockSize> buffer = {};
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
  TemporaryStorageUpstream profiled_upstream{std::pmr::new_delete_resource()};
#endif
  std::optional<std::pmr::monotonic_buffer_resource> resource;

  void Construct() noexcept {
#if defined(HELIOS_MEMORY_ENABLE_PROFILE) && \
    defined(HELIOS_MODULE_PROFILE_AVAILABLE)
    resource.emplace(buffer.data(), buffer.size(), &profiled_upstream);
#else
    resource.emplace(buffer.data(), buffer.size(),
                     std::pmr::new_delete_resource());
#endif
  }

  void Release() noexcept { resource->release(); }
};

enum class SlotState : uint8_t {
  kFree,
  kActive,
  kBusy,
  kBusyRetired,
  kRetired,
};

struct Slot {
  Block block;
  std::atomic<SlotState> state{SlotState::kFree};
};

std::array<Slot, TemporaryStorage::kMaxThreads> g_slots = {};

}  // namespace

struct TemporaryStorage::ThreadBinding {
  ThreadBinding() noexcept : slot_index(ClaimSlot()) {}
  ThreadBinding(const ThreadBinding&) = delete;
  ThreadBinding(ThreadBinding&&) = delete;
  ~ThreadBinding() noexcept;

  ThreadBinding& operator=(const ThreadBinding&) = delete;
  ThreadBinding& operator=(ThreadBinding&&) = delete;

  size_t slot_index = 0;
};

TemporaryStorage::ThreadBinding::~ThreadBinding() noexcept {
  // Heap-free TLS teardown: CAS only. Never overwrite kBusy with kRetired
  // while ResetAll() is inside Release(), that would let ClaimSlot reclaim
  // the slot concurrently. kBusyRetired tells ResetAll to publish kRetired
  // after Release() finishes.
  Slot& slot = g_slots[slot_index];
  for (;;) {
    auto current = slot.state.load(std::memory_order_acquire);
    if (current == SlotState::kActive) {
      if (slot.state.compare_exchange_weak(current, SlotState::kRetired,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
        return;
      }
      continue;
    }

    if (current == SlotState::kBusy) {
      if (slot.state.compare_exchange_weak(current, SlotState::kBusyRetired,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
        return;
      }
      continue;
    }
    return;
  }
}

TemporaryStorage::ThreadBinding& TemporaryStorage::LocalBinding() noexcept {
  thread_local ThreadBinding binding;
  return binding;
}

size_t TemporaryStorage::ClaimSlot() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::ClaimSlot");

  for (size_t i = 0; i < g_slots.size(); ++i) {
    Slot& slot = g_slots[i];

    auto expected = SlotState::kFree;
    if (slot.state.compare_exchange_strong(expected, SlotState::kActive,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
      slot.block.Construct();
      return i;
    }

    expected = SlotState::kRetired;
    if (slot.state.compare_exchange_strong(expected, SlotState::kActive,
                                           std::memory_order_acq_rel,
                                           std::memory_order_acquire)) {
      // Reclaiming a dead thread's slot on the living claimant: overflow
      // blocks from the previous occupant are freed here, never in a TLS
      // destructor.
      slot.block.Release();
      return i;
    }
  }

  HELIOS_VERIFY(false,
                "TemporaryStorage slot pool exhausted (kMaxThreads={}); "
                "increase TemporaryStorage::kMaxThreads",
                kMaxThreads);
  std::abort();
}

std::pmr::memory_resource& TemporaryStorage::Instance() noexcept {
  return *g_slots[LocalBinding().slot_index].block.resource;
}

void TemporaryStorage::Reset() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::Reset");
  g_slots[LocalBinding().slot_index].block.Release();
}

void TemporaryStorage::ResetAll() noexcept {
  HELIOS_MEMORY_PROFILE_SCOPE_N("helios::mem::TemporaryStorage::ResetAll");

  for (Slot& slot : g_slots) {
    auto expected = SlotState::kActive;
    if (!slot.state.compare_exchange_strong(expected, SlotState::kBusy,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
      continue;
    }

    slot.block.Release();

    // Restore kActive iff we still hold kBusy. Failure means the owner
    // CAS-ed kBusy -> kBusyRetired; publish kRetired so ClaimSlot can
    // reclaim after this Release() has finished.
    expected = SlotState::kBusy;
    if (!slot.state.compare_exchange_strong(expected, SlotState::kActive,
                                            std::memory_order_release,
                                            std::memory_order_acquire)) {
      slot.state.store(SlotState::kRetired, std::memory_order_release);
    }
  }
}

}  // namespace helios::mem
