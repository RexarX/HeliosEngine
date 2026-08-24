#pragma once

#include <helios/container/details/callable_buffer_common.hpp>

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

namespace helios::container {

/**
 * @brief Inline storage for heterogeneous callable instances.
 * @details Stores multiple callable instances of heterogeneous types in a
 * single contiguous byte buffer, with embedded function pointers for
 * type-erased invocation. Optimized for fast sequential iteration without
 * virtual dispatch or per-callable heap allocations. Preserves insertion order.
 * @tparam Signatures Function signatures in the form `void(Args...)`.
 */
template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
class CallableBufferArray {
private:
  /// @brief Pack of signature types for use in method validation.
  using SignaturePack = std::tuple<Signatures...>;

public:
  static constexpr size_t kNumOperations = sizeof...(Signatures);
  using size_type = size_t;

  constexpr CallableBufferArray() = default;

  /**
   * @brief Constructs with a PMR memory resource.
   * @param resource Memory resource used for internal storage
   */
  explicit constexpr CallableBufferArray(std::pmr::memory_resource* resource)
      : buffer_(resource), offsets_(resource) {}

  CallableBufferArray(std::nullptr_t) = delete;

  constexpr CallableBufferArray(const CallableBufferArray&) = delete;
  constexpr CallableBufferArray(CallableBufferArray&& other) noexcept
      : buffer_(std::move(other.buffer_)),
        offsets_(std::move(other.offsets_)) {}

  ~CallableBufferArray() noexcept { Clear(); }

  CallableBufferArray& operator=(const CallableBufferArray&) = delete;
  constexpr CallableBufferArray& operator=(
      CallableBufferArray&& other) noexcept;

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
   * @details Empty arrays use `vector::reserve`. Arrays with live callables
   * relocate through `GrowBuffer` so non-trivial objects are not memcpy'd.
   * @param bytes Number of bytes to reserve
   */
  constexpr void ReserveBytes(size_type bytes);

  /**
   * @brief Reserves space for approximately count callables.
   * @details Estimates average callable size for reservation.
   * @param count Approximate number of callables
   */
  constexpr void Reserve(size_type count);

  /**
   * @brief Releases unused capacity in the byte buffer and offset table.
   * @details Relocates live callables into a tightly sized buffer when the
   * current capacity exceeds used bytes.
   */
  constexpr void ShrinkToFit();

  /**
   * @brief Swaps contents with another array.
   * @param other Array to swap with
   */
  constexpr void Swap(CallableBufferArray& other) noexcept;
  friend constexpr void swap(CallableBufferArray& lhs,
                             CallableBufferArray& rhs) noexcept {
    lhs.Swap(rhs);
  }

  /**
   * @brief Merges another array into this one by moving its callables.
   * @details Moves all callables from the other array into this one.
   * The other array is cleared after the operation.
   * @param other Array to merge from
   */
  constexpr void Merge(CallableBufferArray&& other);

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
   * @brief Returns the memory resource used for internal storage.
   * @return Memory resource passed to the constructor, or the default resource
   */
  [[nodiscard]] constexpr std::pmr::memory_resource* GetMemoryResource()
      const noexcept {
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
  using OffsetsType = std::pmr::vector<size_type>;

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

  static size_type AlignOffset(const void* base, size_type offset,
                               size_type alignment) noexcept {
    const auto address = reinterpret_cast<uintptr_t>(base) + offset;
    return offset + ((alignment - (address % alignment)) % alignment);
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr auto CallableBufferArray<Signatures...>::operator=(
    CallableBufferArray&& other) noexcept -> CallableBufferArray& {
  if (this == &other) [[unlikely]] {
    return *this;
  }

  Clear();
  if (GetMemoryResource() == other.GetMemoryResource()) {
    buffer_ = std::move(other.buffer_);
    offsets_ = std::move(other.offsets_);
  } else {
    Merge(std::move(other));
  }

  return *this;
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBufferArray<Signatures...>::Clear() noexcept {
  // Index-based reverse destroy: `std::reverse_iterator` decrements `end()`,
  // which is UBSan-undefined when a PMR vector is empty (`end()` is nullptr).
  const size_type count = offsets_.size();
  auto* const offset_data = offsets_.data();
  auto* const buffer_data = buffer_.data();
  if (count > 0 && offset_data != nullptr && buffer_data != nullptr) {
    for (size_type i = count; i > 0; --i) {
      const auto header_offset = offset_data[i - 1];
      auto* header_base = buffer_data + header_offset;

      auto destroy_fn = *std::launder(reinterpret_cast<DestroyFn*>(
          header_base + (kNumOperations * sizeof(FirstExecuteFn))));

      if (destroy_fn != nullptr) {
        auto* data = GetDataPtr(header_offset);
        destroy_fn(data);
      }
    }
  }

  buffer_.clear();
  offsets_.clear();
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <size_t N, typename... UArgs>
  requires details::ArgsConvertibleTo<
               details::NthSignatureArgsT<N, Signatures...>, UArgs...> &&
           (N < sizeof...(Signatures))
inline void CallableBufferArray<Signatures...>::Invoke(
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArray<Signatures...>::Reserve(size_type count) {
  offsets_.reserve(count);
  buffer_.reserve(count * (BaseHeaderSize() + 32));
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArray<Signatures...>::ReserveBytes(
    size_type bytes) {
  if (bytes <= buffer_.capacity()) {
    return;
  }
  if (Empty()) {
    buffer_.reserve(bytes);
    return;
  }
  GrowBuffer(bytes);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArray<Signatures...>::ShrinkToFit() {
  offsets_.shrink_to_fit();

  if (Empty()) {
    buffer_.clear();
    buffer_.shrink_to_fit();
    return;
  }

  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  constexpr size_t header_alignment = alignof(std::max_align_t);
  const auto old_start = offsets_.front();
  const auto content_size = buffer_.size() - old_start;

  BufferType new_buffer(buffer_.get_allocator());
  new_buffer.reserve(content_size + header_alignment - 1);
  const auto new_start = AlignOffset(new_buffer.data(), 0, header_alignment);
  new_buffer.resize(new_start + content_size);
  std::memcpy(new_buffer.data() + new_start, buffer_.data() + old_start,
              content_size);

  for (auto& header_offset : offsets_) {
    auto* old_header = buffer_.data() + header_offset;
    const auto new_offset = new_start + (header_offset - old_start);
    auto* new_header = new_buffer.data() + new_offset;

    auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
        old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

    if (relocate_fn != nullptr) {
      const auto data_offset = *std::launder(reinterpret_cast<size_type*>(
          old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
          sizeof(RelocateFn)));
      relocate_fn(new_header + data_offset, old_header + data_offset);
    }
    header_offset = new_offset;
  }

  buffer_ = std::move(new_buffer);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArray<Signatures...>::Swap(
    CallableBufferArray& other) noexcept {
  if (this == &other) [[unlikely]] {
    return;
  }

  if (GetMemoryResource() != other.GetMemoryResource()) {
    CallableBufferArray temporary(GetMemoryResource());
    temporary = std::move(*this);
    *this = std::move(other);
    other = std::move(temporary);
    return;
  }

  buffer_.swap(other.buffer_);
  offsets_.swap(other.offsets_);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
constexpr void CallableBufferArray<Signatures...>::Merge(
    CallableBufferArray&& other) {
  if (this == &other) [[unlikely]] {
    return;
  }

  if (other.Empty()) {
    return;
  }
  if (Empty() && GetMemoryResource() == other.GetMemoryResource()) {
    // Direct transfer is safe only when PMR resources match.
    buffer_ = std::move(other.buffer_);
    offsets_ = std::move(other.offsets_);
    return;
  }

  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  constexpr size_t header_alignment = alignof(std::max_align_t);
  const auto source_start = other.offsets_.front();
  const auto content_size = other.buffer_.size() - source_start;

  if (buffer_.capacity() == 0) {
    GrowBuffer(content_size + header_alignment - 1);
  }

  auto aligned_boundary =
      AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
  auto total_required = aligned_boundary + content_size;
  if (total_required > buffer_.capacity()) {
    GrowBuffer(total_required);
    aligned_boundary =
        AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
    total_required = aligned_boundary + content_size;
  }

  if (aligned_boundary > buffer_.size()) {
    buffer_.resize(aligned_boundary, std::byte{0});
  }

  for (const auto offset : other.offsets_) {
    offsets_.push_back(aligned_boundary + (offset - source_start));
  }

  buffer_.resize(total_required);
  std::memcpy(buffer_.data() + aligned_boundary,
              other.buffer_.data() + source_start, content_size);

  for (const auto other_offset : other.offsets_) {
    auto* old_header = other.buffer_.data() + other_offset;
    auto* new_header =
        buffer_.data() + aligned_boundary + (other_offset - source_start);

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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t N, typename... Args>
constexpr void CallableBufferArray<Signatures...>::ExecuteDefault(
    Args... args, void* data) noexcept {
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
constexpr void CallableBufferArray<Signatures...>::ExecuteMethod(
    Args... args, void* data) noexcept {
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
constexpr void CallableBufferArray<Signatures...>::RelocateCallable(
    void* dest, void* src) noexcept {
  T* typed_src = static_cast<T*>(src);
  std::construct_at(static_cast<T*>(dest), std::move(*typed_src));
  std::destroy_at(typed_src);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void CallableBufferArray<Signatures...>::GrowBuffer(
    size_type required_capacity) {
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  constexpr size_t header_alignment = alignof(std::max_align_t);

  BufferType new_buffer(buffer_.get_allocator());
  const auto new_cap = std::max(required_capacity + header_alignment - 1,
                                buffer_.capacity() * 2);
  new_buffer.reserve(new_cap);

  const auto used = buffer_.size();
  if (used > 0) {
    const auto old_start = offsets_.front();
    const auto content_size = used - old_start;
    const auto new_start = AlignOffset(new_buffer.data(), 0, header_alignment);
    new_buffer.resize(new_start + content_size);
    std::memcpy(new_buffer.data() + new_start, buffer_.data() + old_start,
                content_size);

    for (auto& header_offset : offsets_) {
      auto* old_header = buffer_.data() + header_offset;
      const auto new_offset = new_start + (header_offset - old_start);
      auto* new_header = new_buffer.data() + new_offset;

      auto relocate_fn = *std::launder(reinterpret_cast<RelocateFn*>(
          old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn)));

      if (relocate_fn != nullptr) {
        const auto data_offset = *std::launder(reinterpret_cast<size_type*>(
            old_header + (kNumOperations * fn_ptr_size) + sizeof(DestroyFn) +
            sizeof(RelocateFn)));
        relocate_fn(new_header + data_offset, old_header + data_offset);
      }
      header_offset = new_offset;
    }
  }

  buffer_ = std::move(new_buffer);
}

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T>
inline void CallableBufferArray<Signatures...>::PushImpl(T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_t element_size = sizeof(DecayedT);
  constexpr size_t element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  constexpr size_t header_alignment = alignof(std::max_align_t);
  constexpr size_type data_offset_from_header =
      AlignUp(base_header, element_align);
  constexpr size_type total_entry_size = data_offset_from_header + element_size;

  if (buffer_.capacity() == 0) {
    GrowBuffer(total_entry_size + header_alignment - 1);
  }
  auto header_offset =
      AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
  auto required_size = header_offset + total_entry_size;
  if (required_size > buffer_.capacity()) {
    GrowBuffer(required_size);
    header_offset =
        AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
    required_size = header_offset + total_entry_size;
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <auto... Methods, typename T>
inline void CallableBufferArray<Signatures...>::PushImplMethods(T&& callable) {
  using DecayedT = std::remove_cvref_t<T>;

  constexpr size_t element_size = sizeof(DecayedT);
  constexpr size_t element_align = alignof(DecayedT);
  constexpr size_type base_header = BaseHeaderSize();
  constexpr size_t fn_ptr_size = sizeof(FirstExecuteFn);
  constexpr size_t header_alignment = alignof(std::max_align_t);
  constexpr size_type data_offset_from_header =
      AlignUp(base_header, element_align);
  constexpr size_type total_entry_size = data_offset_from_header + element_size;

  if (buffer_.capacity() == 0) {
    GrowBuffer(total_entry_size + header_alignment - 1);
  }
  auto header_offset =
      AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
  auto required_size = header_offset + total_entry_size;
  if (required_size > buffer_.capacity()) {
    GrowBuffer(required_size);
    header_offset =
        AlignOffset(buffer_.data(), buffer_.size(), header_alignment);
    required_size = header_offset + total_entry_size;
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, size_t... Indices>
inline void CallableBufferArray<Signatures...>::StoreFunctionPointersDefault(
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
template <typename T, auto... Methods, size_t... Indices>
inline void CallableBufferArray<Signatures...>::StoreFunctionPointersMethods(
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

template <typename... Signatures>
  requires((sizeof...(Signatures) > 0) &&
           (details::VoidSignature<Signatures> && ...))
inline void* CallableBufferArray<Signatures...>::GetDataPtr(
    size_type header_offset) const noexcept {
  auto* header_base = buffer_.data() + header_offset;
  const auto data_offset = *std::launder(reinterpret_cast<const size_type*>(
      header_base + (kNumOperations * sizeof(FirstExecuteFn)) +
      sizeof(DestroyFn) + sizeof(RelocateFn)));
  return const_cast<std::byte*>(buffer_.data() + header_offset + data_offset);
}

}  // namespace helios::container
