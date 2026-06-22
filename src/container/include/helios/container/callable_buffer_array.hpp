#pragma once

#include <helios/container/details/callable_buffer_common.hpp>

#include <concepts>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <memory_resource>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace helios::container {

/**
 * @brief Implementation class for an inline callable array buffer with explicit
 * allocator.
 * @details Stores multiple callable instances of heterogeneous types in a
 * single contiguous byte buffer, with embedded function pointers for
 * type-erased invocation. Optimized for fast sequential iteration without
 * virtual dispatch or per-callable heap allocations. Preserves insertion order.
 *
 * Use the `CallableBufferArray` alias for ergonomic usage.
 *
 * @tparam Allocator Allocator type for the internal byte buffer (default:
 * `std::allocator<std::byte>`).
 * @tparam Signatures Function signatures in the form void(Args...).
 */
template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
class CallableBufferArrayImpl {
public:
  static constexpr size_t kNumOperations = sizeof...(Signatures);

  using allocator_type = Allocator;
  using byte_allocator_type =
      std::allocator_traits<allocator_type>::template rebind_alloc<std::byte>;
  using offset_allocator_type =
      std::allocator_traits<allocator_type>::template rebind_alloc<size_t>;

  using size_type = size_t;

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

  using BufferType = std::vector<std::byte, byte_allocator_type>;
  using OffsetsType = std::vector<size_type, offset_allocator_type>;

  /// @brief Pack of signature types for use in method validation.
  using SignaturePack = std::tuple<Signatures...>;

public:
  constexpr CallableBufferArrayImpl() = default;

  /**
   * @brief Constructs with a custom allocator.
   * @param alloc Allocator instance to use
   */
  explicit constexpr CallableBufferArrayImpl(const allocator_type& alloc)
      : buffer_(byte_allocator_type(alloc)),
        offsets_(offset_allocator_type(alloc)) {}

  /**
   * @brief Constructs with a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit constexpr CallableBufferArrayImpl(
      std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : CallableBufferArrayImpl(allocator_type{resource}) {}

  CallableBufferArrayImpl(std::nullptr_t) = delete;

  constexpr CallableBufferArrayImpl(const CallableBufferArrayImpl&) = delete;
  constexpr CallableBufferArrayImpl(CallableBufferArrayImpl&& other) noexcept
      : buffer_(std::move(other.buffer_)),
        offsets_(std::move(other.offsets_)) {}

  ~CallableBufferArrayImpl() noexcept { Clear(); }

  CallableBufferArrayImpl& operator=(const CallableBufferArrayImpl&) = delete;
  CallableBufferArrayImpl& operator=(CallableBufferArrayImpl&& other) noexcept;

  /**
   * @brief Clears all stored callables, calling destructors as needed.
   * @details Destroys in reverse insertion order.
   */
  void Clear() noexcept;

  /**
   * @brief Pushes a callable using its default `operator()` for invocation.
   * @details For single-signature arrays, uses `operator()(Args...)`.
   * For multi-signature arrays, uses
   * `operator()(std::integral_constant<size_t, N>, Args...)`.
   * @tparam T Callable type that must be invocable with the appropriate
   * signature(s) and return void.
   * @param callable Callable object to store
   */
  template <CallableBufferStorable T>
    requires details::DefaultOpCallable<std::remove_cvref_t<T>, Signatures...>
  void Push(T&& callable) {
    PushImpl(std::forward<T>(callable));
  }

  /**
   * @brief Pushes a callable with custom member function pointers.
   * @details Allows specifying custom function names for each operation.
   * Number of Methods must match `kNumOperations`. Each method must be
   * invocable on `T` with the corresponding signature's arguments.
   * @tparam Methods Member function pointers for each operation
   * @tparam T Callable type (deduced)
   * @param callable Callable object to store
   *
   * @code
   * // Single operation
   * array.Push<&MyCmd::Run>(MyCmd{});
   *
   * // Multiple operations
   * array.Push<&MyCmd::Execute, &MyCmd::Log>(MyCmd{});
   * @endcode
   */
  template <auto... Methods, CallableBufferStorable T>
    requires details::AllMethodsValid<T, SignaturePack, Methods...> &&
             (sizeof...(Methods) == kNumOperations)
  void Push(T&& callable) {
    PushImplMethods<Methods...>(std::forward<T>(callable));
  }

  /**
   * @brief Invokes operation `N` on all stored callables in insertion order.
   * @details Supports polymorphic argument conversion for flexible invocation.
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
   * @brief Reserves bytes in the internal buffer.
   * @param bytes Number of bytes to reserve
   */
  constexpr void ReserveBytes(size_type bytes) { buffer_.reserve(bytes); }

  /**
   * @brief Reserves space for approximately count callables.
   * @details Estimates average callable size for reservation.
   * @param count Approximate number of callables
   */
  constexpr void Reserve(size_type count);

  /**
   * @brief Swaps contents with another array.
   * @param other Array to swap with
   */
  constexpr void Swap(CallableBufferArrayImpl& other) noexcept(
      std::is_nothrow_swappable_v<BufferType> &&
      std::is_nothrow_swappable_v<OffsetsType>);

  friend void swap(CallableBufferArrayImpl& lhs,
                   CallableBufferArrayImpl&
                       rhs) noexcept(std::is_nothrow_swappable_v<BufferType> &&
                                     std::is_nothrow_swappable_v<OffsetsType>) {
    lhs.Swap(rhs);
  }

  /**
   * @brief Merges another array into this one by moving its callables.
   * @details Moves all callables from the other array into this one.
   * The other array is cleared after the operation.
   * @param other Array to merge from
   */
  constexpr void Merge(CallableBufferArrayImpl&& other);

  /**
   * @brief Checks if the array is empty.
   * @return true if empty, false otherwise
   */
  [[nodiscard]] constexpr bool Empty() const noexcept {
    return offsets_.empty();
  }

  /**
   * @brief Gets the number of stored callables.
   * @return Number of callables
   */
  [[nodiscard]] constexpr size_type Size() const noexcept {
    return offsets_.size();
  }

  /**
   * @brief Gets the current capacity in bytes of the internal buffer.
   * @return Capacity in bytes
   */
  [[nodiscard]] constexpr size_type CapacityBytes() const noexcept {
    return buffer_.capacity();
  }

  /**
   * @brief Gets the allocator used by the array.
   * @return Allocator instance
   */
  [[nodiscard]] constexpr allocator_type GetAllocator() const noexcept {
    return allocator_type(buffer_.get_allocator());
  }

private:
  /**
   * @brief Header layout per callable entry:
   *   - kNumOperations function pointers (one per signature)
   *   - DestroyFn pointer (or `nullptr` if trivially destructible)
   *   - RelocateFn pointer (or `nullptr` if trivially copyable)
   *   - size_type data_offset (offset from header start to callable data)
   *   - [padding for alignment]
   *   - Callable data
   */
  static constexpr size_type BaseHeaderSize() noexcept {
    return (kNumOperations * sizeof(FirstExecuteFn)) + sizeof(DestroyFn) +
           sizeof(RelocateFn) + sizeof(size_type);
  }

  static constexpr size_type AlignUp(size_type offset,
                                     size_type alignment) noexcept {
    return (offset + alignment - 1) & ~(alignment - 1);
  }

  template <typename T, size_t N, typename... Args>
  static constexpr void ExecuteDefault(Args... args, void* data) noexcept;

  template <typename T, auto Method, typename... Args>
  static constexpr void ExecuteMethod(Args... args, void* data) noexcept;

  template <typename T>
  static void DestroyCallable(void* data) noexcept {
    std::destroy_at(static_cast<T*>(data));
  }

  template <typename T>
  static constexpr void RelocateCallable(void* dest, void* src) noexcept;

  void GrowBuffer(size_type required_capacity);

  template <typename T>
  void PushImpl(T&& callable);

  template <auto... Methods, typename T>
  void PushImplMethods(T&& callable);

  template <typename T, size_t... Indices>
  void StoreFunctionPointersDefault(size_type header_offset,
                                    std::index_sequence<Indices...>) noexcept;

  template <typename T, auto... Methods, size_t... Indices>
  void StoreFunctionPointersMethods(size_type header_offset,
                                    std::index_sequence<Indices...>) noexcept;

  [[nodiscard]] void* GetDataPtr(size_type header_offset) const noexcept;

  BufferType buffer_;
  OffsetsType offsets_;
};

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline auto CallableBufferArrayImpl<Allocator, Signatures...>::operator=(
    CallableBufferArrayImpl&& other) noexcept -> CallableBufferArrayImpl& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Clear();
  buffer_ = std::move(other.buffer_);
  offsets_ = std::move(other.offsets_);
  return *this;
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void
CallableBufferArrayImpl<Allocator, Signatures...>::Clear() noexcept {
  for (auto it = offsets_.rbegin(); it != offsets_.rend(); ++it) {
    const auto header_offset = *it;
    auto* header_base = buffer_.data() + header_offset;

    auto destroy_fn = *std::launder(reinterpret_cast<DestroyFn*>(
        header_base + (kNumOperations * sizeof(FirstExecuteFn))));

    if (destroy_fn != nullptr) {
      auto* data = GetDataPtr(header_offset);
      destroy_fn(data);
    }
  }

  buffer_.clear();
  offsets_.clear();
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <size_t N, typename... UArgs>
  requires details::ArgsConvertibleTo<
               details::NthSignatureArgsT<N, Signatures...>, UArgs...> &&
           (N < sizeof...(Signatures))
inline void CallableBufferArrayImpl<Allocator, Signatures...>::Invoke(
    UArgs&&... args) noexcept {
  using ArgsTuple = details::NthSignatureArgsT<N, Signatures...>;
  using ExecuteFn = details::TupleToFunctionPtrType<ArgsTuple>;

  for (const size_type header_offset : offsets_) {
    auto* header_base = buffer_.data() + header_offset;
    auto exec_fn = *std::launder(reinterpret_cast<ExecuteFn*>(
        header_base + (N * sizeof(FirstExecuteFn))));
    auto* data = GetDataPtr(header_offset);
    exec_fn(std::forward<UArgs>(args)..., data);
  }
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArrayImpl<Allocator, Signatures...>::Reserve(
    size_type count) {
  offsets_.reserve(count);
  buffer_.reserve(count * (BaseHeaderSize() + 32));
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArrayImpl<Allocator, Signatures...>::Swap(
    CallableBufferArrayImpl&
        other) noexcept(std::is_nothrow_swappable_v<BufferType> &&
                        std::is_nothrow_swappable_v<OffsetsType>) {
  buffer_.swap(other.buffer_);
  offsets_.swap(other.offsets_);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArrayImpl<Allocator, Signatures...>::Merge(
    CallableBufferArrayImpl&& other) {
  if (this == &other) [[unlikely]] {
    return;
  }

  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  const auto aligned_boundary =
      AlignUp(buffer_.size(), alignof(FirstExecuteFn));
  const auto padding = aligned_boundary - buffer_.size();
  const auto total_required = buffer_.size() + padding + other.buffer_.size();

  if (total_required > buffer_.capacity()) {
    GrowBuffer(total_required);
  }

  if (padding > 0) {
    buffer_.resize(aligned_boundary, std::byte{0});
  }

  const auto offset_adjustment = aligned_boundary;
  for (const auto offset : other.offsets_) {
    offsets_.push_back(offset + offset_adjustment);
  }
  buffer_.insert(buffer_.end(), std::make_move_iterator(other.buffer_.begin()),
                 std::make_move_iterator(other.buffer_.end()));

  for (const auto other_offset : other.offsets_) {
    auto* old_header = other.buffer_.data() + other_offset;
    auto* new_header = buffer_.data() + other_offset + offset_adjustment;

    auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
        old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

    if (relocate_fn != nullptr) {
      const auto data_offset = *std::launder(reinterpret_cast<size_type*>(
          old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
          sizeof(RelocateFn)));
      relocate_fn(new_header + data_offset, old_header + data_offset);
    }
  }

  other.buffer_.clear();
  other.offsets_.clear();
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t N, typename... Args>
constexpr void
CallableBufferArrayImpl<Allocator, Signatures...>::ExecuteDefault(
    Args... args, void* data) noexcept {
  T* callable = static_cast<T*>(data);
  if constexpr (kNumOperations == 1) {
    std::invoke(*callable, args...);
  } else {
    std::invoke(*callable, std::integral_constant<size_t, N>{}, args...);
  }
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, auto Method, typename... Args>
constexpr void CallableBufferArrayImpl<Allocator, Signatures...>::ExecuteMethod(
    Args... args, void* data) noexcept {
  T* callable = static_cast<T*>(data);
  if constexpr (std::invocable<decltype(Method), T&, Args...>) {
    std::invoke(Method, callable, args...);
  } else {
    std::invoke(Method, args...);
  }
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
constexpr void
CallableBufferArrayImpl<Allocator, Signatures...>::RelocateCallable(
    void* dest, void* src) noexcept {
  T* typed_src = static_cast<T*>(src);
  std::construct_at(static_cast<T*>(dest), std::move(*typed_src));
  std::destroy_at(typed_src);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBufferArrayImpl<Allocator, Signatures...>::GrowBuffer(
    size_type required_capacity) {
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  BufferType new_buffer(buffer_.get_allocator());
  const auto new_cap = std::max(required_capacity, buffer_.capacity() * 2);
  new_buffer.resize(new_cap);

  if (!buffer_.empty()) {
    std::memcpy(new_buffer.data(), buffer_.data(), buffer_.size());
  }

  for (auto header_offset : offsets_) {
    auto* old_header = buffer_.data() + header_offset;
    auto* new_header = new_buffer.data() + header_offset;

    auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
        old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

    if (relocate_fn != nullptr) {
      const auto data_offset = *std::launder(reinterpret_cast<size_type*>(
          old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
          sizeof(RelocateFn)));
      relocate_fn(new_header + data_offset, old_header + data_offset);
    }
  }

  const auto used = buffer_.size();
  buffer_ = std::move(new_buffer);
  buffer_.resize(used);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
inline void CallableBufferArrayImpl<Allocator, Signatures...>::PushImpl(
    T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_type element_size = sizeof(DecayedT);
  constexpr size_type element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  const auto header_offset = AlignUp(buffer_.size(), alignof(FirstExecuteFn));
  const auto unaligned_data_offset = header_offset + base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto data_offset_from_header = aligned_data_offset - header_offset;
  const auto total_entry_size = data_offset_from_header + element_size;
  const auto required_size = header_offset + total_entry_size;

  if (required_size > buffer_.capacity()) {
    GrowBuffer(required_size);
  }

  buffer_.resize(required_size);

  auto* header_base = buffer_.data() + header_offset;

  StoreFunctionPointersDefault<DecayedT>(
      header_offset, std::make_index_sequence<kNumOperations>{});

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
  *data_offset_storage = data_offset_from_header;

  auto* data = header_base + data_offset_from_header;
  std::construct_at(std::launder(reinterpret_cast<DecayedT*>(data)),
                    std::forward<T>(callable));

  offsets_.push_back(header_offset);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <auto... Methods, typename T>
inline void CallableBufferArrayImpl<Allocator, Signatures...>::PushImplMethods(
    T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_type element_size = sizeof(DecayedT);
  constexpr size_type element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  const auto header_offset = AlignUp(buffer_.size(), alignof(FirstExecuteFn));
  const auto unaligned_data_offset = header_offset + base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto data_offset_from_header = aligned_data_offset - header_offset;
  const auto total_entry_size = data_offset_from_header + element_size;
  const auto required_size = header_offset + total_entry_size;

  if (required_size > buffer_.capacity()) {
    GrowBuffer(required_size);
  }

  buffer_.resize(required_size);

  auto* header_base = buffer_.data() + header_offset;

  StoreFunctionPointersMethods<DecayedT, Methods...>(
      header_offset, std::make_index_sequence<kNumOperations>{});

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
  *data_offset_storage = data_offset_from_header;

  auto* data = header_base + data_offset_from_header;
  std::construct_at(std::launder(reinterpret_cast<DecayedT*>(data)),
                    std::forward<T>(callable));

  offsets_.push_back(header_offset);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t... Indices>
inline void
CallableBufferArrayImpl<Allocator, Signatures...>::StoreFunctionPointersDefault(
    size_type header_offset, std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data() + header_offset;

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

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, auto... Methods, size_t... Indices>
inline void
CallableBufferArrayImpl<Allocator, Signatures...>::StoreFunctionPointersMethods(
    size_type header_offset, std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data() + header_offset;

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

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void* CallableBufferArrayImpl<Allocator, Signatures...>::GetDataPtr(
    size_type header_offset) const noexcept {
  auto* header_base = buffer_.data() + header_offset;
  const auto data_offset = *std::launder(reinterpret_cast<const size_type*>(
      header_base + (kNumOperations * sizeof(FirstExecuteFn)) +
      sizeof(DestroyFn) + sizeof(RelocateFn)));
  return const_cast<std::byte*>(buffer_.data() + header_offset + data_offset);
}

namespace details {

/// @brief Deduces the `CallableBufferArrayImpl` type from signature arguments.
template <typename... Args>
struct CallableBufferArrayDeducer;

template <VoidSignature FirstSig, typename... RestSigs>
  requires(VoidSignature<RestSigs> && ...)
struct CallableBufferArrayDeducer<FirstSig, RestSigs...> {
  using type =
      CallableBufferArrayImpl<std::allocator<std::byte>, FirstSig, RestSigs...>;
};

template <typename Alloc, VoidSignature FirstSig, typename... RestSigs>
  requires InstantiatedAllocator<Alloc> && (VoidSignature<RestSigs> && ...)
struct CallableBufferArrayDeducer<Alloc, FirstSig, RestSigs...> {
  using type = CallableBufferArrayImpl<Alloc, FirstSig, RestSigs...>;
};

}  // namespace details

/**
 * @brief Inline storage for heterogeneous callable instances with type-erased
 * invocation.
 * @details Stores callable instances of different types in a single contiguous
 * buffer with embedded function pointers for type-safe invocation. Optimized
 * for fast sequential iteration without virtual dispatch or per-callable heap
 * allocations.
 *
 * The allocator parameter is optional. If the first template argument is a
 * function signature (`void(Args...)`), the default allocator is used.
 * Otherwise, the first argument is treated as an allocator.
 *
 * @tparam Args Either signatures only, or allocator followed by signatures
 *
 * @code
 * // Single operation with default allocator (most common usage)
 * CallableBufferArray<void(World&)> commands;
 * commands.Push(SpawnEntityCmd{entity});
 * commands.Invoke(world);
 *
 * // Multiple operations
 * CallableBufferArray<void(World&), void(Logger&)> multi;
 * multi.Push<&Cmd::Execute, &Cmd::Log>(Cmd{data});
 * multi.Invoke<0>(world);
 * multi.Invoke<1>(logger);
 * @endcode
 */
template <typename... Args>
using CallableBufferArray =
    typename details::CallableBufferArrayDeducer<Args...>::type;

/**
 * @brief Inline storage for heterogeneous callable instances with type-erased
 * invocation with polymorphic allocator.
 * @details Stores callable instances of different types in a single contiguous
 * buffer with embedded function pointers for type-safe invocation. Optimized
 * for fast sequential iteration without virtual dispatch or per-callable heap
 * allocations.
 *
 * The allocator parameter is optional. If the first template argument is a
 * function signature (`void(Args...)`), the default allocator is used.
 * Otherwise, the first argument is treated as an allocator.
 *
 * @tparam Args Function signatures in the form void(Args...) for the operations
 * to support.
 *
 * @code
 * // Single operation
 * PmrCallableBufferArray<void(World&)> commands(&resource);
 * commands.Push(SpawnEntityCmd{entity});
 * commands.Invoke(world);
 *
 * // Multiple operations
 * PmrCallableBufferArray<void(World&), void(Logger&)> multi(&resource);
 * multi.Push<&Cmd::Execute, &Cmd::Log>(Cmd{data});
 * multi.Invoke<0>(world);
 * multi.Invoke<1>(logger);
 * @endcode
 */
template <typename... Signatures>
using PmrCallableBufferArray =
    CallableBufferArrayImpl<std::pmr::polymorphic_allocator<std::byte>,
                            Signatures...>;

}  // namespace helios::container
