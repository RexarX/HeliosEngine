#pragma once

#include <helios/assert.hpp>

#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <type_traits>

namespace helios::mem {

template <typename Derived>
class RcFromThis;

template <typename Derived>
class ArcFromThis;

template <typename Derived, typename Allocator = std::allocator<Derived>>
  requires std::derived_from<Derived, RcFromThis<Derived>>
class RefCounted;

template <typename Derived, typename Allocator = std::allocator<Derived>>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
class AtomicRefCounted;

/**
 * @brief CRTP base class that embeds a non-atomic reference counter into the
 * derived type.
 * @details Inherit from this class to make a type usable with `RefCounted<T>`.
 * The counter tracks the number of `RefCounted` handles pointing to the object.
 * The counter is NOT thread-safe; use `ArcFromThis` if thread-safety is
 * required.
 *
 * The counter starts at 0.
 * `RefCounted<T>` increments it on construction and decrements it on
 * destruction, deleting the object when it reaches 0.
 * @warning Do not copy or move `RcFromThis` directly;
 * copies share the same counter slot but should be managed exclusively through
 * `RefCounted<T>`.
 * @tparam Derived The concrete class inheriting from this base
 *
 * @code
 * class Mesh final : public helios::mem::RcFromThis<Mesh> {
 * public:
 *   explicit Mesh(int id) : id_(id) {}
 *   int Id() const { return id_; }
 * private:
 *   int id_;
 * };
 *
 * auto mesh = helios::mem::MakeRc<Mesh>(42);
 * @endcode
 */
template <typename Derived>
class RcFromThis {
public:
  RcFromThis() noexcept = default;
  RcFromThis(const RcFromThis& /*other*/) noexcept {}
  RcFromThis(RcFromThis&& /*other*/) noexcept {}
  ~RcFromThis() noexcept = default;

  RcFromThis& operator=(const RcFromThis& /*other*/) noexcept { return *this; }
  RcFromThis& operator=(RcFromThis&& /*other*/) noexcept { return *this; }

  /**
   * @brief Returns the current reference count.
   * @return Current number of `RefCounted` handles owning this object
   */
  [[nodiscard]] uint32_t RefCount() const noexcept { return ref_count_; }

private:
  template <typename D, typename A>
    requires std::derived_from<D, RcFromThis<D>>
  friend class RefCounted;

  /// @brief Increments the reference count by 1.
  void AddRef() noexcept { ++ref_count_; }

  /**
   * @brief Decrements the reference count and checks if it reached zero.
   * @warning Triggers an assertion if called when the ref count is already
   * zero.
   * @return true if the object should be deleted (ref count reached 0)
   */
  [[nodiscard]] bool Release() noexcept;

  uint32_t ref_count_ = 0;
};

template <typename Derived>
inline bool RcFromThis<Derived>::Release() noexcept {
  HELIOS_ASSERT(ref_count_ > 0,
                "Release() called on object with zero ref count!");
  return --ref_count_ == 0;
}

/**
 * @brief CRTP base class that embeds an atomic reference counter into the
 * derived type.
 * @details Inherit from this class to make a type usable with
 * `AtomicRefCounted<T>`. The counter tracks the number of `AtomicRefCounted`
 * handles pointing to the object. All counter operations use
 * `std::memory_order_acq_rel` / `std::memory_order_acquire` to ensure correct
 * visibility across threads.
 * @note The counter starts at 0. `AtomicRefCounted<T>` increments it on
 * construction and decrements it on destruction, deleting the object when it
 * reaches 0.
 * @warning Do not manage the same raw pointer from multiple threads without
 * using `AtomicRefCounted<T>` handles exclusively.
 * @tparam Derived The concrete class inheriting from this base
 *
 * @code
 * class Texture final : public helios::mem::ArcFromThis<Texture> {
 * public:
 *   explicit Texture(std::string_view name) : name_(name) {}
 *   std::string_view Name() const { return name_; }
 * private:
 *   std::string name_;
 * };
 *
 * auto tex = helios::mem::MakeArc<Texture>("diffuse");
 * @endcode
 */
template <typename Derived>
class ArcFromThis {
public:
  ArcFromThis() noexcept = default;
  ArcFromThis(const ArcFromThis& /*other*/) noexcept {}
  ArcFromThis(ArcFromThis&& /*other*/) noexcept {}
  ~ArcFromThis() noexcept = default;

  ArcFromThis& operator=(const ArcFromThis& /*other*/) noexcept {
    return *this;
  }
  ArcFromThis& operator=(ArcFromThis&& /*other*/) noexcept { return *this; }

  /**
   * @brief Returns the current reference count.
   * @note This is a snapshot; the value may change immediately after the call
   * in a multithreaded context.
   * @return Current number of `AtomicRefCounted` handles owning this object
   */
  [[nodiscard]] uint32_t RefCount() const noexcept {
    return ref_count_.load(std::memory_order_acquire);
  }

private:
  template <typename D, typename A>
    requires std::derived_from<D, ArcFromThis<D>>
  friend class AtomicRefCounted;

  /// @brief Atomically increments the reference count by 1.
  void AddRef() noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  /**
   * @brief Atomically decrements the reference count and checks if it reached
   * zero.
   * @warning Triggers an assertion if called when the ref count is already
   * zero.
   * @return true if the object should be deleted (ref count reached 0)
   */
  [[nodiscard]] bool Release() noexcept;

  std::atomic<uint32_t> ref_count_{0};
};

template <typename Derived>
inline bool ArcFromThis<Derived>::Release() noexcept {
  HELIOS_ASSERT(ref_count_.load(std::memory_order_relaxed) > 0,
                "Release() called on object with zero ref count!");
  return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1;
}

/**
 * @brief Non-atomic intrusive reference-counted smart pointer.
 * @details Manages the lifetime of an object that inherits from
 * `RcFromThis<T>`. The reference counter lives inside the object itself (no
 * separate control block).
 *
 * Semantics are similar to `std::shared_ptr` but:
 * - No heap allocation for the control block.
 * - NOT thread-safe. Use `AtomicRefCounted` / `Arc` for shared ownership across
 * threads.
 * - The managed object MUST inherit from `RcFromThis<T>`.
 *
 * @note A null `RefCounted<T>` is valid and comparable.
 * @warning Not thread-safe. Do NOT share a single `RefCounted<T>` instance
 * across threads. Each thread must hold its own copy of the handle.
 * @tparam Derived Concrete type managed by this handle; must inherit
 * `RcFromThis<Derived>`
 * @tparam Allocator Allocator type used for construction and destruction of the
 * managed object (defaults to `std::allocator<Derived>`)
 *
 * @code
 * // Default allocator
 * auto mesh = helios::mem::MakeRc<Mesh>(42);
 *
 * // Custom pool allocator
 * auto mesh = helios::mem::MakeRc<Mesh, PoolAllocator<Mesh>>(42);
 * auto copy = mesh;                     // ref count: 2
 * mesh.Reset();                         // ref count: 1
 * copy.Reset();                         // ref count: 0 → Mesh deleted via
 * PoolAllocator
 * @endcode
 */
template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
class RefCounted {
public:
  using AllocatorType = Allocator;

  /**
   * @brief Constructs a null handle.
   * @note Only available when `AllocatorType` is default-constructible.
   * For stateful allocators, use `RefCounted(AllocatorType)` or `MakeRcWith`.
   */
  constexpr RefCounted() noexcept
    requires std::default_initializable<AllocatorType>
  = default;

  /**
   * @brief Constructs a null handle storing a pre-built allocator.
   * @details Use this when the allocator is stateful and you need a typed null
   * handle that is ready to receive an assignment from a `MakeRcWith`-created
   * handle.
   */
  constexpr explicit RefCounted(AllocatorType alloc) noexcept
      : alloc_(std::move(alloc)) {}

  /// @brief Constructs a null handle explicitly (default-constructible
  /// allocators only).
  constexpr explicit RefCounted(std::nullptr_t) noexcept
    requires std::default_initializable<AllocatorType>
  {}

  /// @brief Constructs a null handle explicitly with an explicit allocator.
  constexpr RefCounted(std::nullptr_t, AllocatorType alloc) noexcept
      : alloc_(std::move(alloc)) {}

  /**
   * @brief Constructs a null handle with a PMR memory resource.
   * @details Only available when `AllocatorType` is
   * `std::pmr::polymorphic_allocator<T>`.
   * @param resource Polymorphic memory resource to use for allocations
   */
  constexpr explicit RefCounted(std::pmr::memory_resource* resource) noexcept
    requires std::same_as<AllocatorType,
                          std::pmr::polymorphic_allocator<Derived>>
      : alloc_(resource) {}

  /**
   * @brief Constructs from a raw pointer, taking ownership, with a PMR memory
   * resource.
   * @details Only available when `AllocatorType` is
   * `std::pmr::polymorphic_allocator<T>`.
   * @param ptr Raw pointer to take ownership of (may be `nullptr`)
   * @param resource Polymorphic memory resource to use for destruction
   */
  RefCounted(Derived* ptr, std::pmr::memory_resource* resource) noexcept
    requires std::same_as<AllocatorType,
                          std::pmr::polymorphic_allocator<Derived>>
      : RefCounted(ptr, std::pmr::polymorphic_allocator<Derived>(resource)) {}

  /**
   * @brief Constructs from a raw pointer, taking ownership, with an explicit
   * allocator.
   * @warning The pointer must have been allocated with the same allocator and
   * must not be managed by any other owning handle at the time of this call.
   * @param ptr   Raw pointer to take ownership of (may be `nullptr`)
   * @param alloc Allocator instance to use for destruction
   */
  RefCounted(Derived* ptr, AllocatorType alloc) noexcept;

  /**
   * @brief Constructs from a raw pointer using a default-constructed allocator.
   * @note Only available when `AllocatorType` is default-constructible.
   */
  explicit RefCounted(Derived* ptr) noexcept
    requires std::default_initializable<AllocatorType>
      : RefCounted(ptr, AllocatorType{}) {}

  RefCounted(const RefCounted& other) noexcept;
  RefCounted(RefCounted&& other) noexcept;

  // NOLINTBEGIN(hicpp-explicit-conversions)
  // NOLINTBEGIN(google-explicit-constructor)

  /**
   * @brief Converting copy constructor from a compatible (derived) type.
   * @tparam Other Type convertible to `Derived*`
   * @param other Handle to copy from
   */
  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  RefCounted(const RefCounted<Other, Allocator>& other) noexcept;

  /**
   * @brief Converting move constructor from a compatible (derived) type.
   * @tparam Other Type convertible to `Derived*`
   * @param other Handle to move from
   */
  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  RefCounted(RefCounted<Other, Allocator>&& other) noexcept;

  // NOLINTEND(google-explicit-constructor)
  // NOLINTEND(hicpp-explicit-conversions)

  ~RefCounted() noexcept { DecRef(); }

  RefCounted& operator=(const RefCounted& other) noexcept;
  RefCounted& operator=(RefCounted&& other) noexcept;

  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  RefCounted& operator=(const RefCounted<Other, Allocator>& other) noexcept;

  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  RefCounted& operator=(RefCounted<Other, Allocator>&& other) noexcept;

  RefCounted& operator=(std::nullptr_t) noexcept;

  /**
   * @brief Releases ownership and resets the handle to null.
   * @details Decrements the ref count; destroys the object via the stored
   * allocator if it reaches 0.
   */
  void Reset() noexcept;

  /**
   * @brief Releases ownership without decrementing the ref count.
   * @details The caller becomes responsible for managing the object's lifetime.
   * @return Raw pointer that was managed, or `nullptr`
   */
  [[nodiscard]] Derived* Release() noexcept;

  [[nodiscard]] Derived& operator*() const noexcept;
  [[nodiscard]] Derived* operator->() const noexcept;

  [[nodiscard]] explicit operator bool() const noexcept {
    return ptr_ != nullptr;
  }

  [[nodiscard]] bool operator==(const RefCounted& other) const noexcept {
    return ptr_ == other.ptr_;
  }
  [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
    return ptr_ == nullptr;
  }

  template <typename Other>
  [[nodiscard]] bool operator==(
      const RefCounted<Other, Allocator>& other) const noexcept {
    return ptr_ == other.Get();
  }

  /**
   * @brief Checks if this handle is the sole owner of the managed object.
   * @return true if ref count == 1, false if null or ref count > 1
   */
  [[nodiscard]] bool Unique() const noexcept { return RefCount() == 1; }

  /**
   * @brief Checks if the handle is null.
   * @return true if no object is managed
   */
  [[nodiscard]] bool Empty() const noexcept { return ptr_ == nullptr; }

  /**
   * @brief Returns the raw pointer without releasing ownership.
   * @return Raw pointer, or `nullptr` if the handle is null
   */
  [[nodiscard]] Derived* Get() const noexcept { return ptr_; }

  /**
   * @brief Returns the current reference count.
   * @warning Returns 0 if the handle is null.
   * @return Current reference count, or 0 for a null handle
   */
  [[nodiscard]] uint32_t RefCount() const noexcept {
    return ptr_ != nullptr ? ptr_->RcFromThis<Derived>::RefCount() : 0;
  }

  /**
   * @brief Returns a reference to the stored allocator.
   * @return The allocator instance used for this handle
   */
  [[nodiscard]] const AllocatorType& GetAllocator() const noexcept {
    return alloc_;
  }

private:
  // EBO: inherit from AllocatorType so stateless allocators add zero bytes
  // We use a compressed pair pattern manually since we can't inherit from
  // Derived* directly. For simplicity, store the allocator as a member —
  // stateless allocators are typically 1 byte but compilers often optimise them
  // away in practice. A full EBO wrapper can be substituted here if binary size
  // is critical.
  void DecRef() noexcept;

  Derived* ptr_ = nullptr;
  AllocatorType alloc_;
};

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>::RefCounted(Derived* ptr,
                                                  AllocatorType alloc) noexcept
    : ptr_(ptr), alloc_(std::move(alloc)) {
  if (ptr_ != nullptr) [[likely]] {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>::RefCounted(
    const RefCounted& other) noexcept
    : ptr_(other.ptr_),
      alloc_(std::allocator_traits<AllocatorType>::
                 select_on_container_copy_construction(other.alloc_)) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>::RefCounted(RefCounted&& other) noexcept
    : ptr_(other.ptr_), alloc_(std::move(other.alloc_)) {
  other.ptr_ = nullptr;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline RefCounted<Derived, Allocator>::RefCounted(
    const RefCounted<Other, Allocator>& other) noexcept
    : ptr_(other.Get()),
      alloc_(std::allocator_traits<AllocatorType>::
                 select_on_container_copy_construction(other.GetAllocator())) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline RefCounted<Derived, Allocator>::RefCounted(
    RefCounted<Other, Allocator>&& other) noexcept
    : ptr_(other.Release()), alloc_(std::move(other.GetAllocator())) {}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>&
RefCounted<Derived, Allocator>::operator=(const RefCounted& other) noexcept {
  if (this != &other) [[likely]] {
    DecRef();
    if constexpr (std::allocator_traits<AllocatorType>::
                      propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    ptr_ = other.ptr_;
    if (ptr_ != nullptr) {
      ptr_->AddRef();
    }
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>&
RefCounted<Derived, Allocator>::operator=(RefCounted&& other) noexcept {
  if (this != &other) [[likely]] {
    DecRef();
    if constexpr (std::allocator_traits<AllocatorType>::
                      propagate_on_container_move_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline RefCounted<Derived, Allocator>&
RefCounted<Derived, Allocator>::operator=(
    const RefCounted<Other, Allocator>& other) noexcept {
  DecRef();
  if constexpr (std::allocator_traits<AllocatorType>::
                    propagate_on_container_copy_assignment::value) {
    alloc_ = other.GetAllocator();
  }
  ptr_ = other.Get();
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline RefCounted<Derived, Allocator>&
RefCounted<Derived, Allocator>::operator=(
    RefCounted<Other, Allocator>&& other) noexcept {
  DecRef();
  if constexpr (std::allocator_traits<AllocatorType>::
                    propagate_on_container_move_assignment::value) {
    alloc_ = std::move(other.GetAllocator());
  }
  ptr_ = other.Release();
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline RefCounted<Derived, Allocator>&
RefCounted<Derived, Allocator>::operator=(std::nullptr_t) noexcept {
  Reset();
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline void RefCounted<Derived, Allocator>::Reset() noexcept {
  DecRef();
  ptr_ = nullptr;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline Derived* RefCounted<Derived, Allocator>::Release() noexcept {
  auto* raw = ptr_;
  ptr_ = nullptr;
  return raw;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline Derived& RefCounted<Derived, Allocator>::operator*() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null RefCounted!");
  return *ptr_;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline Derived* RefCounted<Derived, Allocator>::operator->() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null RefCounted!");
  return ptr_;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, RcFromThis<Derived>>
inline void RefCounted<Derived, Allocator>::DecRef() noexcept {
  if (ptr_ != nullptr && ptr_->Release()) {
    std::allocator_traits<AllocatorType>::destroy(alloc_, ptr_);
    std::allocator_traits<AllocatorType>::deallocate(alloc_, ptr_, 1);
    ptr_ = nullptr;
  }
}

/**
 * @brief Thread-safe intrusive reference-counted smart pointer.
 * @details Manages the lifetime of an object that inherits from
 * `ArcFromThis<T>`. The reference counter lives inside the object itself (no
 * separate control block).
 *
 * Semantics are similar to `std::shared_ptr` but:
 * - No heap allocation for the control block.
 * - Thread-safe reference counting via `std::atomic<uint32_t>`.
 * - The managed object MUST inherit from `ArcFromThis<T>`.
 *
 * @note A null `AtomicRefCounted<T>` is valid and comparable.
 * Copying/moving the handle itself is NOT atomic.
 * If multiple threads need to share/copy the same handle concurrently, protect
 * the handle with an external lock. What IS thread-safe is having separate
 * `AtomicRefCounted` instances across threads pointing to the same object.
 * @tparam Derived Concrete type managed by this handle; must inherit
 * `ArcFromThis<Derived>`
 * @tparam Allocator Allocator type used for construction and destruction of the
 * managed object (defaults to `std::allocator<Derived>`)
 *
 * @code
 * auto tex = helios::mem::MakeArc<Texture>("diffuse");
 * auto copy = tex;  // thread-safe ref-count increment
 * // Both tex and copy can be used/destroyed from different threads safely.
 * @endcode
 */
template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
class AtomicRefCounted {
public:
  using AllocatorType = Allocator;

  /**
   * @brief Constructs a null handle.
   * @note Only available when `AllocatorType` is default-constructible.
   * For stateful allocators, use `AtomicRefCounted(AllocatorType)` or
   * `MakeArcWith`.
   */
  constexpr AtomicRefCounted() noexcept
    requires std::default_initializable<AllocatorType>
  = default;

  /**
   * @brief Constructs a null handle storing a pre-built allocator.
   * @details Use this when the allocator is stateful and you need a typed null
   * handle that is ready to receive an assignment from a `MakeArcWith`-created
   * handle.
   */
  constexpr explicit AtomicRefCounted(AllocatorType alloc) noexcept
      : alloc_(std::move(alloc)) {}

  /// @brief Constructs a null handle explicitly (default-constructible
  /// allocators only).
  constexpr explicit AtomicRefCounted(std::nullptr_t) noexcept
    requires std::default_initializable<AllocatorType>
  {}

  /// @brief Constructs a null handle explicitly with an explicit allocator.
  constexpr AtomicRefCounted(std::nullptr_t, AllocatorType alloc) noexcept
      : alloc_(std::move(alloc)) {}

  /**
   * @brief Constructs a null handle with a PMR memory resource.
   * @details Only available when `AllocatorType` is
   * `std::pmr::polymorphic_allocator<T>`.
   * @param resource Polymorphic memory resource to use for allocations
   */
  constexpr explicit AtomicRefCounted(
      std::pmr::memory_resource* resource) noexcept
    requires std::same_as<AllocatorType,
                          std::pmr::polymorphic_allocator<Derived>>
      : alloc_(resource) {}

  /**
   * @brief Constructs from a raw pointer, taking ownership, with a PMR memory
   * resource.
   * @details Only available when `AllocatorType` is
   * `std::pmr::polymorphic_allocator<T>`.
   * @param ptr Raw pointer to take ownership of (may be `nullptr`)
   * @param resource Polymorphic memory resource to use for destruction
   */
  AtomicRefCounted(Derived* ptr, std::pmr::memory_resource* resource) noexcept
    requires std::same_as<AllocatorType,
                          std::pmr::polymorphic_allocator<Derived>>
      : AtomicRefCounted(ptr,
                         std::pmr::polymorphic_allocator<Derived>(resource)) {}

  /**
   * @brief Constructs from a raw pointer, taking ownership, with an explicit
   * allocator.
   * @warning The pointer must have been allocated with the same allocator and
   * must not be managed by any other owning handle at the time of this call.
   * @param ptr   Raw pointer to take ownership of (may be `nullptr`)
   * @param alloc Allocator instance to use for destruction
   */
  AtomicRefCounted(Derived* ptr, AllocatorType alloc) noexcept;

  /**
   * @brief Constructs from a raw pointer using a default-constructed allocator.
   * @note Only available when `AllocatorType` is default-constructible.
   */
  explicit AtomicRefCounted(Derived* ptr) noexcept
    requires std::default_initializable<AllocatorType>
      : AtomicRefCounted(ptr, AllocatorType{}) {}

  AtomicRefCounted(const AtomicRefCounted& other) noexcept;
  AtomicRefCounted(AtomicRefCounted&& other) noexcept;

  // NOLINTBEGIN(hicpp-explicit-conversions)
  // NOLINTBEGIN(google-explicit-constructor)

  /**
   * @brief Converting copy constructor from a compatible (derived) type.
   * @tparam Other Type convertible to `Derived*`
   * @param other Handle to copy from
   */
  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  AtomicRefCounted(const AtomicRefCounted<Other, Allocator>& other) noexcept;

  /**
   * @brief Converting move constructor from a compatible (derived) type.
   * @tparam Other Type convertible to `Derived*`
   * @param other Handle to move from
   */
  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  AtomicRefCounted(AtomicRefCounted<Other, Allocator>&& other) noexcept;

  // NOLINTEND(hicpp-explicit-conversions)
  // NOLINTEND(google-explicit-constructor)

  ~AtomicRefCounted() noexcept { DecRef(); }

  AtomicRefCounted& operator=(const AtomicRefCounted& other) noexcept;
  AtomicRefCounted& operator=(AtomicRefCounted&& other) noexcept;

  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  AtomicRefCounted& operator=(
      const AtomicRefCounted<Other, Allocator>& other) noexcept;

  template <typename Other>
    requires std::convertible_to<Other*, Derived*>
  AtomicRefCounted& operator=(
      AtomicRefCounted<Other, Allocator>&& other) noexcept;

  AtomicRefCounted& operator=(std::nullptr_t) noexcept;

  /**
   * @brief Releases ownership and resets the handle to null.
   * @details Decrements the ref count; destroys the object via the stored
   * allocator if it reaches 0.
   */
  void Reset() noexcept;

  /**
   * @brief Releases ownership without decrementing the ref count.
   * @details The caller becomes responsible for managing the object's lifetime.
   * @return Raw pointer that was managed, or `nullptr`
   */
  [[nodiscard]] Derived* Release() noexcept;

  [[nodiscard]] Derived& operator*() const noexcept;
  [[nodiscard]] Derived* operator->() const noexcept;

  [[nodiscard]] explicit operator bool() const noexcept {
    return ptr_ != nullptr;
  }

  [[nodiscard]] bool operator==(const AtomicRefCounted& other) const noexcept {
    return ptr_ == other.ptr_;
  }
  [[nodiscard]] bool operator==(std::nullptr_t) const noexcept {
    return ptr_ == nullptr;
  }

  template <typename Other>
  [[nodiscard]] bool operator==(
      const AtomicRefCounted<Other, Allocator>& other) const noexcept {
    return ptr_ == other.Get();
  }

  /**
   * @brief Checks if this handle is the sole owner of the managed object.
   * @return true if ref count == 1, false if null or ref count > 1
   */
  [[nodiscard]] bool Unique() const noexcept { return RefCount() == 1; }

  /**
   * @brief Checks if the handle is null.
   * @return true if no object is managed
   */
  [[nodiscard]] bool Empty() const noexcept { return ptr_ == nullptr; }

  /**
   * @brief Returns the raw pointer without releasing ownership.
   * @return Raw pointer, or `nullptr` if the handle is null
   */
  [[nodiscard]] Derived* Get() const noexcept { return ptr_; }

  /**
   * @brief Returns the current reference count.
   * @warning Returns 0 if the handle is null.
   * @return Current reference count, or 0 for a null handle
   */
  [[nodiscard]] uint32_t RefCount() const noexcept {
    return ptr_ != nullptr ? ptr_->ArcFromThis<Derived>::RefCount() : 0;
  }

  /**
   * @brief Returns a reference to the stored allocator.
   * @return The allocator instance used for this handle
   */
  [[nodiscard]] const AllocatorType& GetAllocator() const noexcept {
    return alloc_;
  }

private:
  void DecRef() noexcept;

  Derived* ptr_ = nullptr;
  AllocatorType alloc_;
};

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>::AtomicRefCounted(
    Derived* ptr, AllocatorType alloc) noexcept
    : ptr_(ptr), alloc_(std::move(alloc)) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>::AtomicRefCounted(
    const AtomicRefCounted& other) noexcept
    : ptr_(other.ptr_),
      alloc_(std::allocator_traits<AllocatorType>::
                 select_on_container_copy_construction(other.alloc_)) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>::AtomicRefCounted(
    AtomicRefCounted&& other) noexcept
    : ptr_(other.ptr_), alloc_(std::move(other.alloc_)) {
  other.ptr_ = nullptr;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline AtomicRefCounted<Derived, Allocator>::AtomicRefCounted(
    const AtomicRefCounted<Other, Allocator>&
        other) noexcept  // NOLINT(google-explicit-constructor)
    : ptr_(other.Get()),
      alloc_(std::allocator_traits<AllocatorType>::
                 select_on_container_copy_construction(other.GetAllocator())) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline AtomicRefCounted<Derived, Allocator>::AtomicRefCounted(
    AtomicRefCounted<Other, Allocator>&&
        other) noexcept  // NOLINT(google-explicit-constructor)
    : ptr_(other.Release()), alloc_(std::move(other.GetAllocator())) {}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>&
AtomicRefCounted<Derived, Allocator>::operator=(
    const AtomicRefCounted& other) noexcept {
  if (this != &other) [[likely]] {
    DecRef();
    if constexpr (std::allocator_traits<AllocatorType>::
                      propagate_on_container_copy_assignment::value) {
      alloc_ = other.alloc_;
    }
    ptr_ = other.ptr_;
    if (ptr_ != nullptr) {
      ptr_->AddRef();
    }
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>&
AtomicRefCounted<Derived, Allocator>::operator=(
    AtomicRefCounted&& other) noexcept {
  if (this != &other) [[likely]] {
    DecRef();
    if constexpr (std::allocator_traits<AllocatorType>::
                      propagate_on_container_move_assignment::value) {
      alloc_ = std::move(other.alloc_);
    }
    ptr_ = other.ptr_;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline AtomicRefCounted<Derived, Allocator>&
AtomicRefCounted<Derived, Allocator>::operator=(
    const AtomicRefCounted<Other, Allocator>& other) noexcept {
  DecRef();
  if constexpr (std::allocator_traits<AllocatorType>::
                    propagate_on_container_copy_assignment::value) {
    alloc_ = other.GetAllocator();
  }
  ptr_ = other.Get();
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
template <typename Other>
  requires std::convertible_to<Other*, Derived*>
inline AtomicRefCounted<Derived, Allocator>&
AtomicRefCounted<Derived, Allocator>::operator=(
    AtomicRefCounted<Other, Allocator>&& other) noexcept {
  DecRef();
  if constexpr (std::allocator_traits<AllocatorType>::
                    propagate_on_container_move_assignment::value) {
    alloc_ = std::move(other.GetAllocator());
  }
  ptr_ = other.Release();
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline AtomicRefCounted<Derived, Allocator>&
AtomicRefCounted<Derived, Allocator>::operator=(std::nullptr_t) noexcept {
  Reset();
  return *this;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline void AtomicRefCounted<Derived, Allocator>::Reset() noexcept {
  DecRef();
  ptr_ = nullptr;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline Derived* AtomicRefCounted<Derived, Allocator>::Release() noexcept {
  auto* raw = ptr_;
  ptr_ = nullptr;
  return raw;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline Derived& AtomicRefCounted<Derived, Allocator>::operator*()
    const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null AtomicRefCounted!");
  return *ptr_;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline Derived* AtomicRefCounted<Derived, Allocator>::operator->()
    const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null AtomicRefCounted!");
  return ptr_;
}

template <typename Derived, typename Allocator>
  requires std::derived_from<Derived, ArcFromThis<Derived>>
inline void AtomicRefCounted<Derived, Allocator>::DecRef() noexcept {
  if (ptr_ != nullptr && ptr_->Release()) {
    std::allocator_traits<AllocatorType>::destroy(alloc_, ptr_);
    std::allocator_traits<AllocatorType>::deallocate(alloc_, ptr_, 1);
    ptr_ = nullptr;
  }
}

/// @brief Alias for `RefCounted<T>` — non-atomic intrusive reference-counted
/// handle.
template <typename T, typename Allocator = std::allocator<T>>
using Rc = RefCounted<T, Allocator>;

/// @brief Alias for `AtomicRefCounted<T>` — thread-safe intrusive
/// reference-counted handle.
template <typename T, typename Allocator = std::allocator<T>>
using Arc = AtomicRefCounted<T, Allocator>;

/// @brief Alias for `RefCounted<T>` using polymorphic memory allocator.
/// @details Allows using `std::pmr::memory_resource` for dynamic allocator
/// selection.
template <typename T>
using PmrRc = RefCounted<T, std::pmr::polymorphic_allocator<T>>;

/// @brief Alias for `AtomicRefCounted<T>` using polymorphic memory allocator.
/// @details Allows using `std::pmr::memory_resource` for dynamic allocator
/// selection. Thread-safe intrusive reference-counted handle with PMR support.
template <typename T>
using PmrArc = AtomicRefCounted<T, std::pmr::polymorphic_allocator<T>>;

/**
 * @brief Allocates and constructs a `T` object, returning a `Rc<T,
 * std::allocator<T>>` handle.
 * @details Preferred way to create `RefCounted` objects.
 * The object is allocated via `std::allocator<T>` and owned by the returned
 * handle.
 * @tparam T Type to construct; must inherit `RcFromThis<T>`
 * @tparam Args Constructor argument types
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `Rc<T>` with ref count 1
 *
 * @code
 * auto mesh = helios::mem::MakeRc<Mesh>(42);
 * @endcode
 */
template <typename T, typename... Args>
  requires std::derived_from<T, RcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeRc(Args&&... args) -> Rc<T> {
  using AllocT = std::allocator<T>;
  AllocT alloc;
  T* ptr = std::allocator_traits<AllocT>::allocate(alloc, 1);
  std::allocator_traits<AllocT>::construct(alloc, ptr,
                                           std::forward<Args>(args)...);
  return {ptr, std::move(alloc)};
}

/**
 * @brief Allocates and constructs a `T` object, returning an `Arc<T,
 * std::allocator<T>>` handle.
 * @details Preferred way to create `AtomicRefCounted` objects.
 * The object is allocated via `std::allocator<T>` and owned by the returned
 * handle.
 * @note Only available when `std::allocator<T>` is default-constructible (it
 * always is). For stateful allocators (e.g. arena, pool), use `MakeArcWith`
 * instead.
 * @tparam T Type to construct; must inherit `ArcFromThis<T>`
 * @tparam Args Constructor argument types
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `Arc<T>` with ref count 1
 *
 * @code
 * auto tex = helios::mem::MakeArc<Texture>("diffuse");
 * @endcode
 */
template <typename T, typename... Args>
  requires std::derived_from<T, ArcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeArc(Args&&... args) -> Arc<T> {
  using AllocT = std::allocator<T>;
  AllocT alloc;
  T* ptr = std::allocator_traits<AllocT>::allocate(alloc, 1);
  std::allocator_traits<AllocT>::construct(alloc, ptr,
                                           std::forward<Args>(args)...);
  return {ptr, std::move(alloc)};
}

/**
 * @brief Overload of `MakeRc` accepting a pre-constructed allocator instance.
 * @details Use this when your allocator is stateful and must be initialised
 * before the object is created (e.g. a PMR arena).
 * @tparam T Type to construct; must inherit `RcFromThis<T>`
 * @tparam Allocator Concrete allocator type (e.g. `MyAllocator<T>`)
 * @tparam Args Constructor argument types
 * @param alloc Allocator instance to use
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `Rc<T, Allocator>` with ref count 1
 */
template <typename T, typename Allocator, typename... Args>
  requires std::derived_from<T, RcFromThis<T>> &&
           std::constructible_from<T, Args...> &&
           (!std::derived_from<std::remove_pointer_t<Allocator>,
                               std::pmr::memory_resource>)
[[nodiscard]] inline auto MakeRcWith(Allocator alloc, Args&&... args)
    -> Rc<T, Allocator> {
  T* ptr = std::allocator_traits<Allocator>::allocate(alloc, 1);
  std::allocator_traits<Allocator>::construct(alloc, ptr,
                                              std::forward<Args>(args)...);
  return {ptr, std::move(alloc)};
}

/**
 * @brief Allocates and constructs a `T` object using PMR memory resource,
 * returning a `PmrRc<T>` handle.
 * @details Convenience overload that wraps `std::pmr::memory_resource*` in a
 * `std::pmr::polymorphic_allocator<T>`.
 * @tparam T Type to construct; must inherit `RcFromThis<T>`
 * @tparam Args Constructor argument types
 * @param resource PMR memory resource to use for allocation
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `PmrRc<T>` with ref count 1
 *
 * @code
 * auto mesh = helios::mem::MakeRcWith<Mesh>(pmr_resource, 42);
 * @endcode
 */
template <typename T, typename... Args>
  requires std::derived_from<T, RcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeRcWith(std::pmr::memory_resource* resource,
                                     Args&&... args) -> PmrRc<T> {
  return MakeRcWith<T, std::pmr::polymorphic_allocator<T>>(
      std::pmr::polymorphic_allocator<T>(resource),
      std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeRcWith(std::nullptr_t, Args&&...) -> PmrRc<T> = delete;

/**
 * @brief Overload of `MakeArc` accepting a pre-constructed allocator instance.
 * @details Use this when your allocator is stateful and must be initialised
 * before the object is created (e.g. a PMR arena).
 * @tparam T Type to construct; must inherit `ArcFromThis<T>`
 * @tparam Allocator Concrete allocator type (e.g. `MyAllocator<T>`)
 * @tparam Args Constructor argument types
 * @param alloc Allocator instance to use
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `Arc<T, Allocator>` with ref count 1
 */
template <typename T, typename Allocator, typename... Args>
  requires std::derived_from<T, ArcFromThis<T>> &&
           std::constructible_from<T, Args...> &&
           (!std::derived_from<std::remove_pointer_t<Allocator>,
                               std::pmr::memory_resource>)
[[nodiscard]] inline auto MakeArcWith(Allocator alloc, Args&&... args)
    -> Arc<T, Allocator> {
  T* ptr = std::allocator_traits<Allocator>::allocate(alloc, 1);
  std::allocator_traits<Allocator>::construct(alloc, ptr,
                                              std::forward<Args>(args)...);
  return {ptr, std::move(alloc)};
}

/**
 * @brief Allocates and constructs a `T` object using PMR memory resource,
 * returning a `PmrArc<T>` handle.
 * @details Convenience overload that wraps `std::pmr::memory_resource*` in a
 * `std::pmr::polymorphic_allocator<T>`.
 * @tparam T Type to construct; must inherit `ArcFromThis<T>`
 * @tparam Args Constructor argument types
 * @param resource PMR memory resource to use for allocation
 * @param args Arguments forwarded to `T`'s constructor
 * @return A new `PmrArc<T>` with ref count 1
 *
 * @code
 * auto tex = helios::mem::MakeArcWith<Texture>(pmr_resource, "diffuse");
 * @endcode
 */
template <typename T, typename... Args>
  requires std::derived_from<T, ArcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeArcWith(std::pmr::memory_resource* resource,
                                      Args&&... args) -> PmrArc<T> {
  return MakeArcWith<T, std::pmr::polymorphic_allocator<T>>(
      std::pmr::polymorphic_allocator<T>(resource),
      std::forward<Args>(args)...);
}

template <typename T, typename... Args>
auto MakeArcWith(std::nullptr_t, Args&&...) -> PmrRc<T> = delete;

}  // namespace helios::mem
