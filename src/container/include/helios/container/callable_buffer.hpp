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
 * @brief Implementation class for a single-instance callable buffer with
 * explicit allocator.
 * @details Stores exactly one callable of any `CallableBufferStorable` type in
 * a contiguous byte buffer, with embedded function pointers for type-erased
 * invocation. Unlike `CallableBufferArray`, this container holds at most one
 * callable at a time. Designed for command/handler patterns where you want
 * value semantics with type-erased dispatch.
 *
 * Use the `CallableBuffer` alias for ergonomic usage.
 *
 * @tparam Allocator Allocator type for the internal byte buffer (default:
 * `std::allocator<std::byte>`).
 * @tparam Signatures Function signatures in the form void(Args...).
 */
template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
class CallableBufferImpl {
public:
  static constexpr size_t kNumOperations = sizeof...(Signatures);

  using allocator_type = Allocator;
  using byte_allocator_type =
      std::allocator_traits<allocator_type>::template rebind_alloc<std::byte>;
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

  /// @brief Pack of signature types for use in method validation.
  using SignaturePack = std::tuple<Signatures...>;

public:
  /// @brief Default constructor. Creates an empty buffer with no callable.
  CallableBufferImpl() = default;

  /**
   * @brief Constructs with a custom allocator.
   * @param alloc Allocator instance to use
   */
  explicit CallableBufferImpl(const allocator_type& alloc)
      : buffer_(byte_allocator_type(alloc)) {}

  /**
   * @brief Constructs with a PMR memory resource.
   * @details Enabled only when `allocator_type` is constructible from
   * `std::pmr::memory_resource*`.
   * @param resource Memory resource used to construct allocator
   */
  explicit CallableBufferImpl(std::pmr::memory_resource* resource)
    requires std::constructible_from<allocator_type, std::pmr::memory_resource*>
      : CallableBufferImpl(allocator_type{resource}) {}

  CallableBufferImpl(std::nullptr_t) = delete;

  CallableBufferImpl(const CallableBufferImpl&) = delete;
  CallableBufferImpl(CallableBufferImpl&& other) noexcept;
  ~CallableBufferImpl() noexcept { Clear(); }

  CallableBufferImpl& operator=(const CallableBufferImpl&) = delete;
  CallableBufferImpl& operator=(CallableBufferImpl&& other) noexcept;

  /// @brief Destroys the stored callable (if any) and resets the buffer.
  void Clear() noexcept;

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
  void Swap(CallableBufferImpl& other) noexcept(
      std::is_nothrow_swappable_v<BufferType>) {
    buffer_.swap(other.buffer_);
    std::swap(has_value_, other.has_value_);
  }

  friend void swap(CallableBufferImpl& lhs, CallableBufferImpl& rhs) noexcept(
      std::is_nothrow_swappable_v<BufferType>) {
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
   * @brief Gets the allocator used by the buffer.
   * @return Allocator instance
   */
  [[nodiscard]] allocator_type GetAllocator() const noexcept {
    return allocator_type(buffer_.get_allocator());
  }

private:
  static constexpr size_type BaseHeaderSize() noexcept {
    return (kNumOperations * sizeof(FirstExecuteFn)) + sizeof(DestroyFn) +
           sizeof(RelocateFn) + sizeof(size_type);
  }

  static constexpr size_type AlignUp(size_type offset,
                                     size_type alignment) noexcept {
    return (offset + alignment - 1) & ~(alignment - 1);
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
  bool has_value_ = false;
};

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline CallableBufferImpl<Allocator, Signatures...>::CallableBufferImpl(
    CallableBufferImpl&& other) noexcept
    : buffer_(std::move(other.buffer_)), has_value_(other.has_value_) {
  other.has_value_ = false;
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline auto CallableBufferImpl<Allocator, Signatures...>::operator=(
    CallableBufferImpl&& other) noexcept -> CallableBufferImpl& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Clear();
  buffer_ = std::move(other.buffer_);
  has_value_ = other.has_value_;
  other.has_value_ = false;
  return *this;
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBufferImpl<Allocator, Signatures...>::Clear() noexcept {
  if (!has_value_) {
    return;
  }

  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);
  auto* header_base = buffer_.data();

  auto destroy_fn = *std::launder(reinterpret_cast<DestroyFn*>(
      header_base + (kNumOperations * fn_ptr_size)));

  if (destroy_fn != nullptr) {
    auto* data = GetDataPtr();
    destroy_fn(data);
  }

  buffer_.clear();
  has_value_ = false;
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <size_t N, typename... UArgs>
  requires details::ArgsConvertibleTo<
               details::NthSignatureArgsT<N, Signatures...>, UArgs...> &&
           (N < sizeof...(Signatures))
inline void CallableBufferImpl<Allocator, Signatures...>::Invoke(
    UArgs&&... args) noexcept {
  using ArgsTuple = details::NthSignatureArgsT<N, Signatures...>;
  using ExecuteFn = details::TupleToFunctionPtrType<ArgsTuple>;

  auto* header_base = buffer_.data();
  auto exec_fn = *std::launder(
      reinterpret_cast<ExecuteFn*>(header_base + (N * sizeof(FirstExecuteFn))));
  auto* data = GetDataPtr();
  exec_fn(std::forward<UArgs>(args)..., data);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
inline void CallableBufferImpl<Allocator, Signatures...>::SetImpl(
    T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_type element_size = sizeof(DecayedT);
  constexpr size_type element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  // Destroy any existing callable before overwriting
  Clear();

  const auto unaligned_data_offset = base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto total_size = aligned_data_offset + element_size;

  buffer_.resize(total_size);

  auto* header_base = buffer_.data();

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

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <auto... Methods, typename T>
inline void CallableBufferImpl<Allocator, Signatures...>::SetImplMethods(
    T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_type element_size = sizeof(DecayedT);
  constexpr size_type element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);

  Clear();

  const auto unaligned_data_offset = base_header;
  const auto aligned_data_offset =
      AlignUp(unaligned_data_offset, element_align);
  const auto total_size = aligned_data_offset + element_size;

  buffer_.resize(total_size);

  auto* header_base = buffer_.data();

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

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t... Indices>
inline void
CallableBufferImpl<Allocator, Signatures...>::StoreFunctionPointersDefault(
    std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data();

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
CallableBufferImpl<Allocator, Signatures...>::StoreFunctionPointersMethods(
    std::index_sequence<Indices...>) noexcept {
  auto* header_base = buffer_.data();

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
template <typename T, size_t N, typename... Args>
inline void CallableBufferImpl<Allocator, Signatures...>::ExecuteDefault(
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
inline void CallableBufferImpl<Allocator, Signatures...>::ExecuteMethod(
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
inline void CallableBufferImpl<Allocator, Signatures...>::RelocateCallable(
    void* dest, void* src) noexcept {
  T* typed_src = static_cast<T*>(src);
  std::construct_at(static_cast<T*>(dest), std::move(*typed_src));
  std::destroy_at(typed_src);
}

template <typename Allocator, typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void* CallableBufferImpl<Allocator, Signatures...>::GetDataPtr()
    const noexcept {
  constexpr size_type fn_ptr_size = sizeof(FirstExecuteFn);
  auto* header_base = buffer_.data();
  const auto* data_offset_ptr = std::launder(reinterpret_cast<const size_type*>(
      header_base + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
      sizeof(RelocateFn)));
  auto data_offset = *data_offset_ptr;
  return const_cast<std::byte*>(buffer_.data() + data_offset);
}

namespace details {

/// @brief Concept for instantiated allocator types.
template <typename T>
concept InstantiatedAllocator =
    std::is_class_v<T> && !VoidSignature<T> && requires {
      typename std::allocator_traits<T>::template rebind_alloc<std::byte>;
    };

/// @brief Deduces the `CallableBufferImpl` type from signature arguments.
template <typename... Args>
struct CallableBufferDeducer;

template <VoidSignature FirstSig, typename... RestSigs>
  requires(VoidSignature<RestSigs> && ...)
struct CallableBufferDeducer<FirstSig, RestSigs...> {
  using type =
      CallableBufferImpl<std::allocator<std::byte>, FirstSig, RestSigs...>;
};

template <typename Alloc, VoidSignature FirstSig, typename... RestSigs>
  requires InstantiatedAllocator<Alloc> && (VoidSignature<RestSigs> && ...)
struct CallableBufferDeducer<Alloc, FirstSig, RestSigs...> {
  using type = CallableBufferImpl<Alloc, FirstSig, RestSigs...>;
};

}  // namespace details

/**
 * @brief Single-instance callable buffer with type-erased invocation.
 * @details Stores exactly one callable of any type in a contiguous byte buffer
 * with embedded function pointers for type-safe dispatch. Optimized for
 * use-cases where a single command or handler needs to be stored and invoked
 * without virtual dispatch or heap allocation per instance.
 *
 * The allocator parameter is optional. If the first template argument is a
 * function signature (`void(Args...)`), the default allocator is used.
 * Otherwise, the first argument is treated as an allocator.
 *
 * @tparam Args Either signatures only, or allocator followed by signatures
 *
 * @code
 * // Single operation
 * CallableBuffer<void(World&)> cmd;
 * cmd.Set(SpawnEntityCmd{entity});
 * cmd.Invoke(world);
 *
 * // Multiple operations
 * CallableBuffer<void(World&), void(Logger&)> multi_cmd;
 * multi_cmd.Set<&Cmd::Execute, &Cmd::Log>(Cmd{data});
 * multi_cmd.Invoke<0>(world);
 * multi_cmd.Invoke<1>(logger);
 * @endcode
 */
template <typename... Args>
using CallableBuffer = typename details::CallableBufferDeducer<Args...>::type;

/**
 * @brief Single-instance callable buffer with type-erased invocation with
 * polymorphic allocator.
 * @details Stores exactly one callable of any type in a contiguous byte buffer
 * with embedded function pointers for type-safe dispatch. Optimized for
 * use-cases where a single command or handler needs to be stored and invoked
 * without virtual dispatch or heap allocation per instance.
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
 * PmrCallableBuffer<void(World&)> cmd(&resource);
 * cmd.Set(SpawnEntityCmd{entity});
 * cmd.Invoke(world);
 *
 * // Multiple operations
 * PmrCallableBuffer<void(World&), void(Logger&)> multi_cmd(&resource);
 * multi_cmd.Set<&Cmd::Execute, &Cmd::Log>(Cmd{data});
 * multi_cmd.Invoke<0>(world);
 * multi_cmd.Invoke<1>(logger);
 * @endcode
 */
template <typename... Signatures>
using PmrCallableBuffer =
    CallableBufferImpl<std::pmr::polymorphic_allocator<std::byte>,
                       Signatures...>;

}  // namespace helios::container
