#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.container;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <memory_resource>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#endif
#include <helios/assert.hpp>
#include <helios/container/details/callable_buffer_common.hpp>

HELIOS_MODULE_EXPORT
namespace helios::container {

/**
 * @brief Single-instance callable buffer with type-erased invocation.
 * @details Stores exactly one callable of any `CallableBufferStorable` type in
 * a contiguous byte buffer, with embedded function pointers for type-erased
 * invocation. Unlike `CallableBufferArray`, this container holds at most one
 * callable at a time. Designed for command/handler patterns where you want
 * value semantics with type-erased dispatch.
 * @tparam Signatures Function signatures in the form `void(Args...)`.
 */
template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
class CallableBuffer {
private:
  /// @brief Pack of signature types for use in method validation.
  using SignaturePack = std::tuple<Signatures...>;

public:
  static constexpr size_t kNumOperations = sizeof...(Signatures);
  using size_type = size_t;

  /// @brief Default constructor using the default PMR resource.
  CallableBuffer() = default;

  /**
   * @brief Constructs with a PMR memory resource.
   * @param resource Memory resource used for internal storage
   */
  explicit CallableBuffer(std::pmr::memory_resource* resource)
      : buffer_(resource) {}

  CallableBuffer(std::nullptr_t) = delete;

  CallableBuffer(const CallableBuffer&) = delete;
  CallableBuffer(CallableBuffer&& other) noexcept;
  ~CallableBuffer() noexcept { Clear(); }

  CallableBuffer& operator=(const CallableBuffer&) = delete;
  CallableBuffer& operator=(CallableBuffer&& other) noexcept;

  /// @brief Destroys the stored callable (if any) and resets the buffer.
  void Clear() noexcept;

  /**
   * @brief Reserves bytes in the internal buffer.
   * @details Empty buffers use `vector::reserve`. A stored non-trivial
   * callable is relocated instead of memcpy'd if reallocation is required.
   * @param bytes Number of bytes to reserve
   */
  void ReserveBytes(size_type bytes);

  /**
   * @brief Stores a callable using its default `operator()` for invocation.
   * @details For single-signature buffers, uses `operator()(Args...)`.
   * For multi-signature buffers, uses
   * `operator()(std::integral_constant<size_t, N>, Args...)`.
   * Any previously stored callable is destroyed first.
   * @tparam T Callable type
   * @param callable Callable object to store
   */
  template <CallableBufferStorable T>
    requires details::DefaultOpCallable<std::remove_cvref_t<T>, Signatures...>
  void Set(T&& callable) {
    SetImpl(std::forward<T>(callable));
  }

  /**
   * @brief Stores a callable with custom member function pointers.
   * @details Allows specifying custom function names for each operation.
   * Number of Methods must match `kNumOperations`.
   * Any previously stored callable is destroyed first.
   * @tparam Methods Member function pointers for each operation
   * @tparam T Callable type (deduced)
   * @param callable Callable object to store
   *
   * @code
   * // Single operation
   * buffer.Set<&MyCmd::Run>(MyCmd{});
   *
   * // Multiple operations
   * buffer.Set<&MyCmd::Execute, &MyCmd::Log>(MyCmd{});
   * @endcode
   */
  template <auto... Methods, CallableBufferStorable T>
    requires details::AllMethodsValid<T, SignaturePack, Methods...> &&
             (sizeof...(Methods) == kNumOperations)
  void Set(T&& callable) {
    SetImplMethods<Methods...>(std::forward<T>(callable));
  }

  /**
   * @brief Invokes operation `N` on the stored callable.
   * @details Supports polymorphic argument conversion for flexible invocation.
   * @warning Triggers assertion if the buffer is empty.
   * @tparam N Operation index (0 to `kNumOperations-1`)
   * @tparam UArgs Actual argument types (must be convertible to signature args)
   * @param args Arguments for operation `N`
   */
  template <size_t N = 0, typename... UArgs>
    requires details::ArgsConvertibleTo<
                 details::NthSignatureArgsT<N, Signatures...>, UArgs...> &&
             (N < sizeof...(Signatures))
  void Invoke(UArgs&&... args) noexcept;

  /**
   * @brief Swaps contents with another buffer.
   * @param other Buffer to swap with
   */
  void Swap(CallableBuffer& other) noexcept;
  friend void swap(CallableBuffer& lhs, CallableBuffer& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Checks if a callable is currently stored.
   * @return true if empty, false otherwise
   */
  [[nodiscard]] bool Empty() const noexcept { return !has_value_; }

  /**
   * @brief Gets the current capacity in bytes of the internal buffer.
   * @return Capacity in bytes
   */
  [[nodiscard]] size_type CapacityBytes() const noexcept {
    return buffer_.capacity();
  }

  /**
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] std::pmr::memory_resource* GetMemoryResource() const noexcept {
    return buffer_.get_allocator().resource();
  }

private:
  using DestroyFn = void (*)(void*);
  using RelocateFn = void (*)(void* dest, void* src);

  /// @brief Function pointer type for the first signature (used for size
  /// calculations).
  using FirstExecuteFn = details::TupleToFunctionPtrType<
      details::NthSignatureArgsT<0, Signatures...>>;

  static_assert(((sizeof(details::TupleToFunctionPtrType<
                         details::NthSignatureArgsT<0, Signatures...>>) ==
                  sizeof(details::TupleToFunctionPtrType<
                         details::SignatureArgsT<Signatures>>)) &&
                 ...),
                "All function pointer types must have the same size");

  using BufferType = std::pmr::vector<std::byte>;

  static constexpr size_type BaseHeaderSize() noexcept {
    return (kNumOperations * sizeof(FirstExecuteFn)) + sizeof(DestroyFn) +
           sizeof(RelocateFn) + sizeof(size_type);
  }

  static constexpr size_type AlignUp(size_type offset,
                                     size_type alignment) noexcept {
    return (offset + alignment - 1) & ~(alignment - 1);
  }

  static size_type AlignOffset(const void* base, size_type offset,
                               size_type alignment) noexcept {
    const auto address = reinterpret_cast<uintptr_t>(base) + offset;
    return offset + ((alignment - (address % alignment)) % alignment);
  }

  template <typename T, size_t N, typename... Args>
  static void ExecuteDefault(Args... args, void* data) noexcept;

  template <typename T, auto Method, typename... Args>
  static void ExecuteMethod(Args... args, void* data) noexcept;

  template <typename T>
  static void DestroyCallable(void* data) noexcept {
    std::destroy_at(static_cast<T*>(data));
  }

  template <typename T>
  static void RelocateCallable(void* dest, void* src) noexcept;

  template <typename T>
  void SetImpl(T&& callable);

  template <auto... Methods, typename T>
  void SetImplMethods(T&& callable);

  template <typename T, size_t... Indices>
  void StoreFunctionPointersDefault(std::index_sequence<Indices...>) noexcept;

  template <typename T, auto... Methods, size_t... Indices>
  void StoreFunctionPointersMethods(std::index_sequence<Indices...>) noexcept;

  [[nodiscard]] void* GetDataPtr() const noexcept;

  BufferType buffer_;
  size_type header_offset_ = 0;
  bool has_value_ = false;
};

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline CallableBuffer<Signatures...>::CallableBuffer(
    CallableBuffer&& other) noexcept
    : buffer_(std::move(other.buffer_)),
      header_offset_(other.header_offset_),
      has_value_(other.has_value_) {
  other.header_offset_ = 0;
  other.has_value_ = false;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline auto CallableBuffer<Signatures...>::operator=(
    CallableBuffer&& other) noexcept -> CallableBuffer& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Clear();
  if (!other.has_value_) {
    return *this;
  }

  if (GetMemoryResource() == other.GetMemoryResource()) {
    buffer_ = std::move(other.buffer_);
    header_offset_ = other.header_offset_;
    has_value_ = true;
    other.header_offset_ = 0;
    other.has_value_ = false;
    return *this;
  }

  constexpr size_t header_alignment = alignof(std::max_align_t);
  const auto content_size = other.buffer_.size() - other.header_offset_;
  buffer_.resize(content_size + header_alignment - 1);
  header_offset_ = AlignOffset(buffer_.data(), 0, header_alignment);
  buffer_.resize(header_offset_ + content_size);
  std::memcpy(buffer_.data() + header_offset_,
              other.buffer_.data() + other.header_offset_, content_size);

  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
      other.buffer_.data() + other.header_offset_ +
      (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

  has_value_ = true;
  if (relocate_fn != nullptr) {
    relocate_fn(GetDataPtr(), other.GetDataPtr());
    other.has_value_ = false;
    other.buffer_.clear();
    other.header_offset_ = 0;
  } else {
    other.Clear();
  }
  return *this;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBuffer<Signatures...>::Clear() noexcept {
  if (!has_value_) {
    return;
  }

  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  auto* header_base = buffer_.data() + header_offset_;

  auto destroy_fn = *std::launder(reinterpret_cast<DestroyFn*>(
      header_base + (kNumOperations * fn_ptr_size)));

  if (destroy_fn != nullptr) {
    auto* data = GetDataPtr();
    destroy_fn(data);
  }

  buffer_.clear();
  header_offset_ = 0;
  has_value_ = false;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBuffer<Signatures...>::ReserveBytes(size_type bytes) {
  if (bytes <= buffer_.capacity()) {
    return;
  }

  if (!has_value_) {
    buffer_.reserve(bytes);
    return;
  }

  constexpr size_t header_alignment = alignof(std::max_align_t);
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  const auto content_size = buffer_.size() - header_offset_;
  BufferType new_buffer(buffer_.get_allocator());
  new_buffer.reserve(std::max(bytes, content_size + header_alignment - 1));
  const auto new_header_offset =
      AlignOffset(new_buffer.data(), 0, header_alignment);
  new_buffer.resize(new_header_offset + content_size);
  std::memcpy(new_buffer.data() + new_header_offset,
              buffer_.data() + header_offset_, content_size);

  auto* old_header = buffer_.data() + header_offset_;
  auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
      old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

  if (relocate_fn != nullptr) {
    const auto data_offset = *std::launder(reinterpret_cast<size_type*>(
        old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
        sizeof(RelocateFn)));
    relocate_fn(new_buffer.data() + new_header_offset + data_offset,
                old_header + data_offset);
  }

  header_offset_ = new_header_offset;
  buffer_ = std::move(new_buffer);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <size_t N, typename... UArgs>
  requires details::ArgsConvertibleTo<
               details::NthSignatureArgsT<N, Signatures...>, UArgs...> &&
           (N < sizeof...(Signatures))
inline void CallableBuffer<Signatures...>::Invoke(UArgs&&... args) noexcept {
  HELIOS_ASSERT(!Empty(), "Cannot invoke on an empty buffer!");

  using ArgsTuple = details::NthSignatureArgsT<N, Signatures...>;
  using ExecuteFn = details::TupleToFunctionPtrType<ArgsTuple>;

  auto* header_base = buffer_.data() + header_offset_;
  auto exec_fn = *std::launder(
      reinterpret_cast<ExecuteFn*>(header_base + (N * sizeof(FirstExecuteFn))));
  auto* data = GetDataPtr();
  exec_fn(std::forward<UArgs>(args)..., data);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBuffer<Signatures...>::Swap(
    CallableBuffer& other) noexcept {
  if (this == &other) [[unlikely]] {
    return;
  }

  if (GetMemoryResource() != other.GetMemoryResource()) {
    CallableBuffer temporary(GetMemoryResource());
    temporary = std::move(*this);
    *this = std::move(other);
    other = std::move(temporary);
    return;
  }
  buffer_.swap(other.buffer_);
  std::swap(header_offset_, other.header_offset_);
  std::swap(has_value_, other.has_value_);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
inline void CallableBuffer<Signatures...>::SetImpl(T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_t element_size = sizeof(DecayedT);
  constexpr size_t element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);

  // Destroy any existing callable before overwriting
  Clear();

  constexpr size_t header_alignment = alignof(std::max_align_t);
  const auto unaligned_data_offset = base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto total_size = aligned_data_offset + element_size;

  buffer_.resize(total_size + header_alignment - 1);
  header_offset_ = AlignOffset(buffer_.data(), 0, header_alignment);
  buffer_.resize(header_offset_ + total_size);

  auto* header_base = buffer_.data() + header_offset_;

  StoreFunctionPointersDefault<DecayedT>(
      std::make_index_sequence<kNumOperations>{});

  auto* destroy_fn_ptr = std::launder(reinterpret_cast<DestroyFn*>(
      header_base + (kNumOperations * fn_ptr_size)));
  if constexpr (std::is_trivially_destructible_v<DecayedT>) {
    *destroy_fn_ptr = nullptr;
  } else {
    *destroy_fn_ptr = &DestroyCallable<DecayedT>;
  }

  auto* relocate_fn_ptr = std::launder(reinterpret_cast<RelocateFn*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));
  if constexpr (std::is_trivially_copyable_v<DecayedT>) {
    *relocate_fn_ptr = nullptr;
  } else {
    *relocate_fn_ptr = &RelocateCallable<DecayedT>;
  }

  // Store data offset
  auto* data_offset_storage = std::launder(reinterpret_cast<size_type*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
      sizeof(RelocateFn)));
  *data_offset_storage = aligned_data_offset;

  auto* data = header_base + aligned_data_offset;
  std::construct_at(std::launder(reinterpret_cast<DecayedT*>(data)),
                    std::forward<T>(callable));

  has_value_ = true;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <auto... Methods, typename T>
inline void CallableBuffer<Signatures...>::SetImplMethods(T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_t element_size = sizeof(DecayedT);
  constexpr size_t element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);

  Clear();

  constexpr size_t header_alignment = alignof(std::max_align_t);
  const auto unaligned_data_offset = base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto total_size = aligned_data_offset + element_size;

  buffer_.resize(total_size + header_alignment - 1);
  header_offset_ = AlignOffset(buffer_.data(), 0, header_alignment);
  buffer_.resize(header_offset_ + total_size);

  auto* header_base = buffer_.data() + header_offset_;

  StoreFunctionPointersMethods<DecayedT, Methods...>(
      std::make_index_sequence<kNumOperations>{});

  auto* destroy_fn_ptr = std::launder(reinterpret_cast<DestroyFn*>(
      header_base + (kNumOperations * fn_ptr_size)));
  if constexpr (std::is_trivially_destructible_v<DecayedT>) {
    *destroy_fn_ptr = nullptr;
  } else {
    *destroy_fn_ptr = &DestroyCallable<DecayedT>;
  }

  auto* relocate_fn_ptr = std::launder(reinterpret_cast<RelocateFn*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));
  if constexpr (std::is_trivially_copyable_v<DecayedT>) {
    *relocate_fn_ptr = nullptr;
  } else {
    *relocate_fn_ptr = &RelocateCallable<DecayedT>;
  }

  auto* data_offset_storage = std::launder(reinterpret_cast<size_type*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
      sizeof(RelocateFn)));
  *data_offset_storage = aligned_data_offset;

  auto* data = header_base + aligned_data_offset;
  std::construct_at(std::launder(reinterpret_cast<DecayedT*>(data)),
                    std::forward<T>(callable));

  has_value_ = true;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t... Indices>
inline void CallableBuffer<Signatures...>::StoreFunctionPointersDefault(
    std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data() + header_offset_;

  (
      [header_base]<size_t Index>() {
        using ArgsTuple = details::NthSignatureArgsT<Index, Signatures...>;
        using ExecuteFn = details::TupleToFunctionPtrType<ArgsTuple>;

        [header_base]<typename... Args>(std::tuple<Args...>*) {
          auto* fn_ptr = std::launder(reinterpret_cast<ExecuteFn*>(
              header_base + (Index * sizeof(FirstExecuteFn))));
          *fn_ptr = &ExecuteDefault<T, Index, Args...>;
        }(static_cast<ArgsTuple*>(nullptr));
      }.template operator()<Indices>(),
      ...);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, auto... Methods, size_t... Indices>
inline void CallableBuffer<Signatures...>::StoreFunctionPointersMethods(
    std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data() + header_offset_;

  (
      [header_base]<size_t Index>() {
        using ArgsTuple = details::NthSignatureArgsT<Index, Signatures...>;
        using ExecuteFn = details::TupleToFunctionPtrType<ArgsTuple>;

        [header_base]<typename... Args>(std::tuple<Args...>*) {
          auto* fn_ptr = std::launder(reinterpret_cast<ExecuteFn*>(
              header_base + (Index * sizeof(FirstExecuteFn))));
          *fn_ptr = &ExecuteMethod<T, details::kNthMethod<Index, Methods...>,
                                   Args...>;
        }(static_cast<ArgsTuple*>(nullptr));
      }.template operator()<Indices>(),
      ...);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t N, typename... Args>
inline void CallableBuffer<Signatures...>::ExecuteDefault(Args... args,
                                                          void* data) noexcept {
  T* callable = static_cast<T*>(data);
  if constexpr (kNumOperations == 1) {
    std::invoke(*callable, args...);
  } else {
    std::invoke(*callable, std::integral_constant<size_t, N>{}, args...);
  }
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, auto Method, typename... Args>
inline void CallableBuffer<Signatures...>::ExecuteMethod(Args... args,
                                                         void* data) noexcept {
  T* callable = static_cast<T*>(data);
  if constexpr (std::invocable<decltype(Method), T&, Args...>) {
    std::invoke(Method, callable, args...);
  } else {
    std::invoke(Method, args...);
  }
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
inline void CallableBuffer<Signatures...>::RelocateCallable(
    void* dest, void* src) noexcept {
  T* typed_src = static_cast<T*>(src);
  std::construct_at(static_cast<T*>(dest), std::move(*typed_src));
  std::destroy_at(typed_src);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void* CallableBuffer<Signatures...>::GetDataPtr() const noexcept {
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  auto* header_base = buffer_.data() + header_offset_;
  const auto* data_offset_ptr = std::launder(reinterpret_cast<const size_type*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
      sizeof(RelocateFn)));
  auto data_offset = *data_offset_ptr;
  return const_cast<std::byte*>(buffer_.data() + header_offset_ + data_offset);
}

}  // namespace helios::container
#endif  // HELIOS_MODULE_CONSUMER_SHIM
