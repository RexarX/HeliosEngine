#include <pch.hpp>

#include <helios/memory/temporary_storage.hpp>
#include <helios/platform/platform.hpp>

#include <atomic>

namespace helios::mem {

void TemporaryStorage::Init() {
  // The node is heap-allocated and intentionally never deleted.
  registry_node_ = new RegistryNode{};
  registry_node_->owner = this;

  // Treiber-stack push of registry_node_ onto registry_head_.
  // registry_node_->next is written here and never again, so once this
  // node is visible to another thread (via the successful CAS's release,
  // paired with an acquire load of registry_head_ in ResetAll), `next` is
  // guaranteed visible too.
  registry_node_->next = registry_head_.load(std::memory_order_relaxed);
  while (!registry_head_.compare_exchange_weak(
      registry_node_->next, registry_node_, std::memory_order_release,
      std::memory_order_relaxed)) {
    // On failure, compare_exchange_weak has already reloaded the current
    // head into registry_node_->next, so we just retry the CAS with it.
  }
}

void TemporaryStorage::Destroy() noexcept {
  // Wait until ResetAll() has finished any in-flight Reset(), then retire
  // the node so later walks skip `owner`. Do not yield/sleep: this runs from
  // a thread_local destructor, and Darwin can livelock a dying thread that
  // calls sched_yield() during TLS teardown.
  auto expected = NodeState::kActive;
  while (!registry_node_->state.compare_exchange_weak(
      expected, NodeState::kRetired, std::memory_order_acq_rel,
      std::memory_order_acquire)) {
    if (expected == NodeState::kRetired) {
      return;
    }
    expected = NodeState::kActive;
    HELIOS_PAUSE_CPU();
  }
}

void TemporaryStorage::ResetAllImpl() noexcept {
  // Acquire pairs with the release in the constructor's successful CAS:
  // if we observe a node here, we also observe its fully-initialized
  // `next` and `owner`.
  for (RegistryNode* node = registry_head_.load(std::memory_order_acquire);
       node != nullptr; node = node->next) {
    // Try to claim the node. Success means we may call Reset() on owner:
    // Destroy() only proceeds to member teardown after CAS kActive ->
    // kRetired, so it cannot be destroying upstream_resource_ while we
    // hold kBusy. Failure means kBusy (another ResetAll) or kRetired
    // (owner already exited); skip rather than wait.
    auto expected = NodeState::kActive;
    if (node->state.compare_exchange_strong(expected, NodeState::kBusy,
                                            std::memory_order_acq_rel,
                                            std::memory_order_acquire)) {
      node->owner->Reset();
      node->state.store(NodeState::kActive, std::memory_order_release);
    }
  }
}

}  // namespace helios::mem
