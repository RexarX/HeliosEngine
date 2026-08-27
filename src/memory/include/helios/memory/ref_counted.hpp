#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.memory;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <utility>
#endif
#include <helios/assert.hpp>

HELIOS_MODULE_EXPORT
namespace helios::mem {

template <typename Derived>
class RefCounted;

template <typename Derived>
class AtomicRefCounted;

/**
 * @brief CRTP base embedding a non-atomic intrusive reference counter.
 * @tparam Derived Concrete type inheriting from this base
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
   * @return Number of owning handles
   */
  [[nodiscard]] uint32_t RefCount() const noexcept { return ref_count_; }

private:
  template <typename>
  friend class RefCounted;

  void AddRef() noexcept { ++ref_count_; }

  /**
   * @brief Decrements the count and reports whether the object is unowned.
   * @warning Triggers an assertion if the count is already zero.
   * @return True when the count reached zero
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
 * @brief CRTP base embedding an atomic intrusive reference counter.
 * @tparam Derived Concrete type inheriting from this base
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
   * @brief Returns a snapshot of the current reference count.
   * @return Number of owning handles at the time of the call
   */
  [[nodiscard]] uint32_t RefCount() const noexcept {
    return ref_count_.load(std::memory_order_acquire);
  }

private:
  template <typename>
  friend class AtomicRefCounted;

  void AddRef() noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  /**
   * @brief Decrements the count and reports whether the object is unowned.
   * @warning Triggers an assertion if the count is already zero.
   * @return True when the count reached zero
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
 * @brief Non-atomic PMR intrusive reference-counted handle.
 * @details The memory resource is part of the handle state and is propagated
 * by every copy, move, and assignment. The last handle destroys and
 * deallocates the object through the resource that allocated it.
 * @warning The managed type must inherit from `RcFromThis<Derived>`.
 * @tparam Derived Managed object type
 */
template <typename Derived>
class RefCounted {
  static_assert(std::derived_from<Derived, RcFromThis<Derived>>);

public:
  using allocator_type = std::pmr::polymorphic_allocator<Derived>;

  /// @brief Constructs a null handle using the default memory resource.
  RefCounted() noexcept = default;

  /// @brief Constructs a null handle using the default memory resource.
  explicit RefCounted(std::nullptr_t) noexcept {}

  /**
   * @brief Constructs a null handle with a specific memory resource.
   * @param resource Resource retained by the handle
   */
  explicit RefCounted(std::pmr::memory_resource* resource) noexcept
      : resource_(resource) {}

  /**
   * @brief Takes ownership of a PMR-allocated object.
   * @warning `ptr` must have been allocated by `resource`.
   * @param ptr Object to own, or null
   * @param resource Resource used to destroy and deallocate the object
   */
  RefCounted(Derived* ptr, std::pmr::memory_resource* resource) noexcept;

  /**
   * @brief Takes ownership using the default memory resource.
   * @warning `ptr` must have been allocated by the default memory resource.
   * @param ptr Object to own, or null
   */
  explicit RefCounted(Derived* ptr) noexcept
      : RefCounted(ptr, std::pmr::get_default_resource()) {}

  RefCounted(const RefCounted& other) noexcept;
  RefCounted(RefCounted&& other) noexcept;
  ~RefCounted() noexcept { DecRef(); }

  RefCounted& operator=(const RefCounted& other) noexcept;
  RefCounted& operator=(RefCounted&& other) noexcept;
  RefCounted& operator=(std::nullptr_t) noexcept;

  /// @brief Releases this handle's ownership.
  void Reset() noexcept;

  /**
   * @brief Releases the pointer without decrementing its reference count.
   * @return Previously managed pointer, or null
   */
  [[nodiscard]] Derived* Release() noexcept;

  /**
   * @brief Dereferences the managed object.
   * @warning Triggers an assertion when the handle is null.
   * @return Managed object
   */
  [[nodiscard]] Derived& operator*() const noexcept;

  /**
   * @brief Accesses the managed object.
   * @warning Triggers an assertion when the handle is null.
   * @return Managed object pointer
   */
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
  [[nodiscard]] allocator_type GetAllocator() const noexcept {
    return allocator_type(resource_);
  }

  /// @brief Returns the handle's memory resource.
  [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept {
    return resource_;
  }

private:
  void DecRef() noexcept;

  Derived* ptr_ = nullptr;
  std::pmr::memory_resource* resource_ = std::pmr::get_default_resource();
};

template <typename Derived>
inline RefCounted<Derived>::RefCounted(
    Derived* ptr, std::pmr::memory_resource* resource) noexcept
    : ptr_(ptr), resource_(resource) {
  HELIOS_ASSERT(resource_ != nullptr, "RefCounted resource cannot be null!");
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived>
inline RefCounted<Derived>::RefCounted(const RefCounted& other) noexcept
    : ptr_(other.ptr_), resource_(other.resource_) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived>
inline RefCounted<Derived>::RefCounted(RefCounted&& other) noexcept
    : ptr_(other.ptr_), resource_(other.resource_) {
  other.ptr_ = nullptr;
}

template <typename Derived>
inline RefCounted<Derived>& RefCounted<Derived>::operator=(
    const RefCounted& other) noexcept {
  if (this != &other) {
    if (other.ptr_ != nullptr) {
      other.ptr_->AddRef();
    }
    DecRef();
    ptr_ = other.ptr_;
    resource_ = other.resource_;
  }
  return *this;
}

template <typename Derived>
inline RefCounted<Derived>& RefCounted<Derived>::operator=(
    RefCounted&& other) noexcept {
  if (this != &other) {
    DecRef();
    ptr_ = other.ptr_;
    resource_ = other.resource_;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename Derived>
inline RefCounted<Derived>& RefCounted<Derived>::operator=(
    std::nullptr_t) noexcept {
  Reset();
  return *this;
}

template <typename Derived>
inline void RefCounted<Derived>::Reset() noexcept {
  DecRef();
  ptr_ = nullptr;
}

template <typename Derived>
inline Derived* RefCounted<Derived>::Release() noexcept {
  auto* raw = ptr_;
  ptr_ = nullptr;
  return raw;
}

template <typename Derived>
inline Derived& RefCounted<Derived>::operator*() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null RefCounted!");
  return *ptr_;
}

template <typename Derived>
inline Derived* RefCounted<Derived>::operator->() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null RefCounted!");
  return ptr_;
}

template <typename Derived>
inline void RefCounted<Derived>::DecRef() noexcept {
  if (ptr_ != nullptr && ptr_->Release()) {
    allocator_type allocator(resource_);
    std::allocator_traits<allocator_type>::destroy(allocator, ptr_);
    std::allocator_traits<allocator_type>::deallocate(allocator, ptr_, 1);
    ptr_ = nullptr;
  }
}

/**
 * @brief Atomic PMR intrusive reference-counted handle.
 * @details The memory resource is part of the handle state and is propagated
 * by every copy, move, and assignment. Separate handles may be released safely
 * on different threads.
 * @warning The managed type must inherit from `ArcFromThis<Derived>`.
 * @tparam Derived Managed object type
 */
template <typename Derived>
class AtomicRefCounted {
  static_assert(std::derived_from<Derived, ArcFromThis<Derived>>);

public:
  using AllocatorType = std::pmr::polymorphic_allocator<Derived>;

  /// @brief Constructs a null handle using the default memory resource.
  AtomicRefCounted() noexcept = default;

  /// @brief Constructs a null handle using the default memory resource.
  explicit AtomicRefCounted(std::nullptr_t) noexcept {}

  /**
   * @brief Constructs a null handle with a specific memory resource.
   * @param resource Resource retained by the handle
   */
  explicit AtomicRefCounted(std::pmr::memory_resource* resource) noexcept
      : resource_(resource) {}

  /**
   * @brief Takes ownership of a PMR-allocated object.
   * @warning `ptr` must have been allocated by `resource`.
   * @param ptr Object to own, or null
   * @param resource Resource used to destroy and deallocate the object
   */
  AtomicRefCounted(Derived* ptr, std::pmr::memory_resource* resource) noexcept;

  /**
   * @brief Takes ownership using the default memory resource.
   * @warning `ptr` must have been allocated by the default memory resource.
   * @param ptr Object to own, or null
   */
  explicit AtomicRefCounted(Derived* ptr) noexcept
      : AtomicRefCounted(ptr, std::pmr::get_default_resource()) {}

  AtomicRefCounted(const AtomicRefCounted& other) noexcept;
  AtomicRefCounted(AtomicRefCounted&& other) noexcept;
  ~AtomicRefCounted() noexcept { DecRef(); }

  AtomicRefCounted& operator=(const AtomicRefCounted& other) noexcept;
  AtomicRefCounted& operator=(AtomicRefCounted&& other) noexcept;
  AtomicRefCounted& operator=(std::nullptr_t) noexcept;

  /// @brief Releases this handle's ownership.
  void Reset() noexcept;

  /**
   * @brief Releases the pointer without decrementing its reference count.
   * @return Previously managed pointer, or null
   */
  [[nodiscard]] Derived* Release() noexcept;

  /**
   * @brief Dereferences the managed object.
   * @warning Triggers an assertion when the handle is null.
   * @return Managed object
   */
  [[nodiscard]] Derived& operator*() const noexcept;

  /**
   * @brief Accesses the managed object.
   * @warning Triggers an assertion when the handle is null.
   * @return Managed object pointer
   */
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
   * @brief Returns reference to the stored allocator.
   * @return The allocator instance used for this handle
   */
  [[nodiscard]] AllocatorType GetAllocator() const noexcept {
    return AllocatorType(resource_);
  }

  /**
   * @brief Returns poinetr to the stored memory resource.
   * @return The memory resource instance used for this handle
   */
  [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept {
    return resource_;
  }

private:
  void DecRef() noexcept;

  Derived* ptr_ = nullptr;
  std::pmr::memory_resource* resource_ = std::pmr::get_default_resource();
};

template <typename Derived>
inline AtomicRefCounted<Derived>::AtomicRefCounted(
    Derived* ptr, std::pmr::memory_resource* resource) noexcept
    : ptr_(ptr), resource_(resource) {
  HELIOS_ASSERT(resource_ != nullptr,
                "AtomicRefCounted resource cannot be null!");
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived>
inline AtomicRefCounted<Derived>::AtomicRefCounted(
    const AtomicRefCounted& other) noexcept
    : ptr_(other.ptr_), resource_(other.resource_) {
  if (ptr_ != nullptr) {
    ptr_->AddRef();
  }
}

template <typename Derived>
inline AtomicRefCounted<Derived>::AtomicRefCounted(
    AtomicRefCounted&& other) noexcept
    : ptr_(other.ptr_), resource_(other.resource_) {
  other.ptr_ = nullptr;
}

template <typename Derived>
inline AtomicRefCounted<Derived>& AtomicRefCounted<Derived>::operator=(
    const AtomicRefCounted& other) noexcept {
  if (this != &other) [[likely]] {
    if (other.ptr_ != nullptr) {
      other.ptr_->AddRef();
    }
    DecRef();
    ptr_ = other.ptr_;
    resource_ = other.resource_;
  }
  return *this;
}

template <typename Derived>
inline AtomicRefCounted<Derived>& AtomicRefCounted<Derived>::operator=(
    AtomicRefCounted&& other) noexcept {
  if (this != &other) [[likely]] {
    DecRef();
    ptr_ = other.ptr_;
    resource_ = other.resource_;
    other.ptr_ = nullptr;
  }
  return *this;
}

template <typename Derived>
inline AtomicRefCounted<Derived>& AtomicRefCounted<Derived>::operator=(
    std::nullptr_t) noexcept {
  Reset();
  return *this;
}

template <typename Derived>
inline void AtomicRefCounted<Derived>::Reset() noexcept {
  DecRef();
  ptr_ = nullptr;
}

template <typename Derived>
inline Derived* AtomicRefCounted<Derived>::Release() noexcept {
  auto* raw = ptr_;
  ptr_ = nullptr;
  return raw;
}

template <typename Derived>
inline Derived& AtomicRefCounted<Derived>::operator*() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null AtomicRefCounted!");
  return *ptr_;
}

template <typename Derived>
inline Derived* AtomicRefCounted<Derived>::operator->() const noexcept {
  HELIOS_ASSERT(ptr_ != nullptr, "Dereferencing null AtomicRefCounted!");
  return ptr_;
}

template <typename Derived>
inline void AtomicRefCounted<Derived>::DecRef() noexcept {
  if (ptr_ != nullptr && ptr_->Release()) {
    AllocatorType allocator(resource_);
    std::allocator_traits<AllocatorType>::destroy(allocator, ptr_);
    std::allocator_traits<AllocatorType>::deallocate(allocator, ptr_, 1);
    ptr_ = nullptr;
  }
}

/// @brief Non-atomic intrusive reference-counted handle.
template <typename T>
using Rc = RefCounted<T>;

/// @brief Atomic intrusive reference-counted handle.
template <typename T>
using Arc = AtomicRefCounted<T>;

/**
 * @brief Allocates an object with a specific PMR memory resource.
 * @warning Triggers an assertion when `resource` is null.
 * @tparam T Object type
 * @tparam Args Constructor argument types
 * @param resource Resource used for allocation and eventual deallocation
 * @param args Arguments forwarded to the object constructor
 * @return Owning handle with reference count one
 */
template <typename T, typename... Args>
  requires std::derived_from<T, RcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeRcWith(std::pmr::memory_resource* resource,
                                     Args&&... args) -> Rc<T> {
  HELIOS_ASSERT(resource != nullptr, "MakeRcWith resource cannot be null!");
  std::pmr::polymorphic_allocator<T> allocator(resource);
  T* ptr = std::allocator_traits<decltype(allocator)>::allocate(allocator, 1);
  std::allocator_traits<decltype(allocator)>::construct(
      allocator, ptr, std::forward<Args>(args)...);
  return {ptr, resource};
}

template <typename T, typename... Args>
auto MakeRcWith(std::nullptr_t, Args&&...) -> Rc<T> = delete;

/**
 * @brief Allocates an object with the default PMR memory resource.
 * @tparam T Object type
 * @tparam Args Constructor argument types
 * @param args Arguments forwarded to the object constructor
 * @return Owning handle with reference count one
 */
template <typename T, typename... Args>
  requires std::derived_from<T, RcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeRc(Args&&... args) -> Rc<T> {
  return MakeRcWith<T>(std::pmr::get_default_resource(),
                       std::forward<Args>(args)...);
}

/**
 * @brief Atomically reference-counts an object allocated with a PMR resource.
 * @warning Triggers an assertion when `resource` is null.
 * @tparam T Object type
 * @tparam Args Constructor argument types
 * @param resource Resource used for allocation and eventual deallocation
 * @param args Arguments forwarded to the object constructor
 * @return Owning handle with reference count one
 */
template <typename T, typename... Args>
  requires std::derived_from<T, ArcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeArcWith(std::pmr::memory_resource* resource,
                                      Args&&... args) -> Arc<T> {
  HELIOS_ASSERT(resource != nullptr, "MakeArcWith resource cannot be null!");
  std::pmr::polymorphic_allocator<T> allocator(resource);
  T* ptr = std::allocator_traits<decltype(allocator)>::allocate(allocator, 1);
  std::allocator_traits<decltype(allocator)>::construct(
      allocator, ptr, std::forward<Args>(args)...);
  return {ptr, resource};
}

template <typename T, typename... Args>
auto MakeArcWith(std::nullptr_t, Args&&...) -> Arc<T> = delete;

/**
 * @brief Allocates an atomically reference-counted object with the default PMR
 * resource.
 * @tparam T Object type
 * @tparam Args Constructor argument types
 * @param args Arguments forwarded to the object constructor
 * @return Owning handle with reference count one
 */
template <typename T, typename... Args>
  requires std::derived_from<T, ArcFromThis<T>> &&
           std::constructible_from<T, Args...>
[[nodiscard]] inline auto MakeArc(Args&&... args) -> Arc<T> {
  return MakeArcWith<T>(std::pmr::get_default_resource(),
                        std::forward<Args>(args)...);
}

}  // namespace helios::mem
#endif  // HELIOS_MODULE_CONSUMER_SHIM
