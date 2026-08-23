#include <pch.hpp>

#include <helios/memory/temporary_storage.hpp>

#include <atomic>
#include <thread>

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
  // Claim exclusive access to this node before letting member destruction
  // (upstream_resource_'s teardown, which runs immediately after this
  // function body returns) proceed.
  auto expected = NodeState::kActive;
  while (!registry_node_->state.compare_exchange_weak(
      expected, NodeState::kBusy, std::memory_order_acq_rel,
      std::memory_order_relaxed)) {
    expected = NodeState::kActive;
    std::this_thread::yield();
  }

  // We now hold kBusy exclusively. upstream_resource_'s destructor runs
  // implicitly right after this function body, tearing down the chunk
  // list with no other thread able to touch it concurrently.
  registry_node_->state.store(NodeState::kRetired, std::memory_order_release);
}

void TemporaryStorage::ResetAllImpl() noexcept {
  // Acquire pairs with the release in the constructor's successful CAS:
  // if we observe a node here, we also observe its fully-initialized
  // `next` and `owner`.
  for (RegistryNode* node = registry_head_.load(std::memory_order_acquire);
       node != nullptr; node = node->next) {
    // Try to claim the node. Success means we have exclusive rights to
    // call Reset() on its owner, in particular, the owning thread's
    // destructor cannot be mid-teardown of the same upstream_resource_,
    // because it would have had to win this same CAS first. Failure means
    // the node is either already kBusy (most likely: its owner is
    // concurrently exiting) or kRetired; either way we skip it rather than
    // wait.
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
