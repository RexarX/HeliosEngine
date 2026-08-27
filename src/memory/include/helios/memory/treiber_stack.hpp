#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.memory;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <atomic>
#include <cstdint>
#endif

HELIOS_MODULE_EXPORT
namespace helios::mem {

/**
 * @brief Lock-free intrusive Treiber stack.
 * @details Nodes are untyped `void*` values. The first `sizeof(void*)` bytes of
 * each node store the successor pointer and are accessed with
 * `std::atomic_ref`. The stack head is a 128-bit `{pointer, tag}` word so the
 * ABA tag is a full native integer and does not wrap after 65536 operations.
 *
 * The stack never allocates or frees nodes. Callers own node storage for the
 * lifetime of every pointer currently in the stack.
 *
 * @note `Push` and `Pop` are safe to call concurrently. `Clear` and move
 * operations require exclusive access.
 *
 * @code
 * struct Node {
 *   Node* next = nullptr;
 *   int value = 0;
 * };
 *
 * helios::mem::TreiberStack stack;
 * Node a{.value = 1};
 * Node b{.value = 2};
 * stack.Push(&a);
 * stack.Push(&b);
 * Node* top = static_cast<Node*>(stack.Pop());
 * @endcode
 */
class alignas(16) TreiberStack {
public:
  TreiberStack() noexcept = default;
  TreiberStack(const TreiberStack&) = delete;
  TreiberStack(TreiberStack&& other) noexcept { Store(other.Exchange(Head{})); }
  ~TreiberStack() noexcept = default;

  TreiberStack& operator=(const TreiberStack&) = delete;
  TreiberStack& operator=(TreiberStack&& other) noexcept;

  /**
   * @brief Pushes `node` as the new top.
   * @warning Triggers assertion when `node` is null or not aligned to
   * `alignof(void*)`.
   * @param node Node whose first word stores the intrusive next pointer
   */
  void Push(void* node) noexcept;

  /**
   * @brief Removes and returns the top node.
   * @return The previous top, or `nullptr` when the stack is empty
   */
  [[nodiscard]] void* Pop() noexcept;

  /**
   * @brief Drops the head without visiting or freeing nodes.
   * @warning Must not run concurrently with `Push` or `Pop`. Callers still own
   * every node that was in the stack.
   */
  void Clear() noexcept { Store(Head{}); }

  /**
   * @brief Returns true when the stack has no nodes.
   * @return True if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return Top() == nullptr; }

  /**
   * @brief Returns the current top without removing it.
   * @return Top node, or `nullptr` when empty
   */
  [[nodiscard]] void* Top() const noexcept { return Load().ptr; }

  /**
   * @brief Returns the successor stored in `node`'s first word.
   * @warning Triggers assertion when `node` is null or not aligned to
   * `alignof(void*)`.
   * @param node Node currently or previously linked in a Treiber stack
   * @return Successor pointer, or `nullptr` at the end of the chain
   */
  [[nodiscard]] static void* Next(const void* node) noexcept;

private:
  struct alignas(16) Head {
    void* ptr = nullptr;
    uintptr_t tag = 0;
  };

  static_assert(sizeof(Head) == 16,
                "TreiberStack::Head must be a 16-byte CAS word!");
  static_assert(
      alignof(Head) == 16,
      "TreiberStack::Head must be 16-byte aligned for lock-free CAS!");

  [[nodiscard]] Head Load() const noexcept;
  void Store(Head head) noexcept;
  Head Exchange(Head head) noexcept;
  bool CompareExchangeWeak(Head& expected, Head desired,
                           std::memory_order success,
                           std::memory_order failure) noexcept;

#ifdef __SIZEOF_INT128__
  using Raw = unsigned __int128;

  [[nodiscard]] static Raw ToRaw(Head head) noexcept;
  [[nodiscard]] static Head FromRaw(Raw raw) noexcept;

  mutable std::atomic<Raw> raw_{0};
#else
  [[nodiscard]] Head LoadSeqCst() const noexcept;
  bool CompareExchangeSeqCst(Head& expected, Head desired) noexcept;

  mutable volatile long long words_[2] = {};
#endif
};

inline TreiberStack& TreiberStack::operator=(TreiberStack&& other) noexcept {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Store(other.Exchange(Head{}));
  return *this;
}

}  // namespace helios::mem
#endif  // HELIOS_MODULE_CONSUMER_SHIM
