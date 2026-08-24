#include <pch.hpp>

#include <helios/memory/treiber_stack.hpp>

#include <helios/assert.hpp>
#include <helios/memory/common.hpp>

#include <atomic>

#if defined(_MSC_VER) && !defined(__SIZEOF_INT128__)
#include <intrin.h>
#endif

namespace helios::mem {

namespace {

[[nodiscard]] auto NodeNext(void* node) noexcept -> std::atomic_ref<void*> {
  return std::atomic_ref(*static_cast<void**>(node));
}

void AssertValidNode(const void* node) noexcept {
  HELIOS_ASSERT(node != nullptr, "Treiber stack node must not be null!");
  HELIOS_ASSERT(IsAligned(node, alignof(void*)),
                "Treiber stack node '{}' must be aligned to '{}'!", node,
                alignof(void*));
}

}  // namespace

void TreiberStack::Push(void* node) noexcept {
  AssertValidNode(node);

  Head observed = Load();
  for (;;) {
    NodeNext(node).store(observed.ptr, std::memory_order_relaxed);
    const Head desired{.ptr = node, .tag = observed.tag + 1};
    if (CompareExchangeWeak(observed, desired, std::memory_order_release,
                            std::memory_order_acquire)) {
      return;
    }
  }
}

void* TreiberStack::Pop() noexcept {
  Head observed = Load();
  for (;;) {
    void* const node = observed.ptr;
    if (node == nullptr) {
      return nullptr;
    }

    void* const next = NodeNext(node).load(std::memory_order_relaxed);
    const Head desired{.ptr = next, .tag = observed.tag + 1};
    if (CompareExchangeWeak(observed, desired, std::memory_order_acq_rel,
                            std::memory_order_acquire)) {
      return node;
    }
  }
}

void* TreiberStack::Next(const void* node) noexcept {
  AssertValidNode(node);
  return NodeNext(const_cast<void*>(node)).load(std::memory_order_acquire);
}

#ifdef __SIZEOF_INT128__

auto TreiberStack::ToRaw(Head head) noexcept -> Raw {
  return static_cast<Raw>(reinterpret_cast<uintptr_t>(head.ptr)) |
         (static_cast<Raw>(head.tag) << 64);
}

auto TreiberStack::FromRaw(Raw raw) noexcept -> Head {
  return {
      .ptr = reinterpret_cast<void*>(static_cast<uintptr_t>(raw)),
      .tag = static_cast<uintptr_t>(raw >> 64),
  };
}

auto TreiberStack::Load() const noexcept -> Head {
  return FromRaw(raw_.load(std::memory_order_acquire));
}

void TreiberStack::Store(Head head) noexcept {
  raw_.store(ToRaw(head), std::memory_order_release);
}

auto TreiberStack::Exchange(Head head) noexcept -> Head {
  return FromRaw(raw_.exchange(ToRaw(head), std::memory_order_acq_rel));
}

bool TreiberStack::CompareExchangeWeak(Head& expected, Head desired,
                                       std::memory_order success,
                                       std::memory_order failure) noexcept {
  Raw observed = ToRaw(expected);
  const bool ok =
      raw_.compare_exchange_weak(observed, ToRaw(desired), success, failure);
  expected = FromRaw(observed);
  return ok;
}

#else

auto TreiberStack::LoadSeqCst() const noexcept -> Head {
  long long comparand[2] = {0, 0};
  _InterlockedCompareExchange128(words_, 0, 0, comparand);
  return {
      .ptr = reinterpret_cast<void*>(comparand[0]),
      .tag = static_cast<uintptr_t>(comparand[1]),
  };
}

bool TreiberStack::CompareExchangeSeqCst(Head& expected,
                                         Head desired) noexcept {
  long long comparand[2] = {reinterpret_cast<long long>(expected.ptr),
                            static_cast<long long>(expected.tag)};
  const unsigned char ok = _InterlockedCompareExchange128(
      words_, static_cast<long long>(desired.tag),
      reinterpret_cast<long long>(desired.ptr), comparand);
  expected.ptr = reinterpret_cast<void*>(comparand[0]);
  expected.tag = static_cast<uintptr_t>(comparand[1]);
  return ok != 0;
}

auto TreiberStack::Load() const noexcept -> Head {
  return LoadSeqCst();
}

void TreiberStack::Store(Head head) noexcept {
  Head expected = LoadSeqCst();
  while (!CompareExchangeSeqCst(expected, head)) {
  }
}

auto TreiberStack::Exchange(Head head) noexcept -> Head {
  Head expected = LoadSeqCst();
  while (!CompareExchangeSeqCst(expected, head)) {
  }
  return expected;
}

bool TreiberStack::CompareExchangeWeak(Head& expected, Head desired,
                                       std::memory_order /*success*/,
                                       std::memory_order /*failure*/) noexcept {
  return CompareExchangeSeqCst(expected, desired);
}

#endif

}  // namespace helios::mem
