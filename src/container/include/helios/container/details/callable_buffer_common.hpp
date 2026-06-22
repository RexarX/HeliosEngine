#pragma once

#include <helios/utils/common_traits.hpp>

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace helios::container {

namespace details {

/// @brief Concept for void(Args...) function signatures.
template <typename Sig>
concept VoidSignature = requires {
  []<typename... Args>(void (*)(Args...)) {}(static_cast<Sig*>(nullptr));
};

/// @brief Extracts argument types from a void(Args...) signature.
template <typename Sig>
struct SignatureArgsHelper;

template <typename... Args>
struct SignatureArgsHelper<void(Args...)> {
  using Type = std::tuple<Args...>;
  static constexpr size_t kCount = sizeof...(Args);
};

template <typename Sig>
using SignatureArgsT = typename SignatureArgsHelper<Sig>::Type;

/// @brief Gets the Nth signature from a pack of signatures.
template <size_t N, typename First, typename... Rest>
struct NthSignatureHelper {
  using Type = typename NthSignatureHelper<N - 1, Rest...>::Type;
};

template <typename First, typename... Rest>
struct NthSignatureHelper<0, First, Rest...> {
  using Type = First;
};

template <size_t N, typename... Sigs>
  requires(N < sizeof...(Sigs))
using NthSignatureT = typename NthSignatureHelper<N, Sigs...>::Type;

/// @brief Gets the argument tuple for the Nth signature.
template <size_t N, typename... Sigs>
using NthSignatureArgsT = SignatureArgsT<NthSignatureT<N, Sigs...>>;

/// @brief Converts a tuple of argument types to a function pointer type.
template <typename ArgsTuple>
struct TupleToFunctionPtrHelper;

template <typename... Args>
struct TupleToFunctionPtrHelper<std::tuple<Args...>> {
  using Type = void (*)(Args..., void*);
};

template <typename ArgsTuple>
using TupleToFunctionPtrType =
    typename TupleToFunctionPtrHelper<ArgsTuple>::Type;

/// @brief Gets the Nth element from a non-type template parameter pack.
template <size_t N, auto First, auto... Rest>
struct NthMethodHelper {
  static constexpr auto kValue = NthMethodHelper<N - 1, Rest...>::kValue;
};

template <auto First, auto... Rest>
struct NthMethodHelper<0, First, Rest...> {
  static constexpr auto kValue = First;
};

template <size_t N, auto... Methods>
  requires(N < sizeof...(Methods))
inline constexpr auto kNthMethod = NthMethodHelper<N, Methods...>::kValue;

/// @brief Concept checking if UArgs are polymorphically convertible to the
/// signature's args.
template <typename ArgsTuple, typename... UArgs>
concept ArgsConvertibleTo = []<typename... SigArgs>(std::tuple<SigArgs...>*) {
  return (sizeof...(SigArgs) == sizeof...(UArgs)) &&
         (sizeof...(SigArgs) == 0 ||
          (utils::PolymorphicConvertible<UArgs, SigArgs> && ...));
}(static_cast<ArgsTuple*>(nullptr));

/// @brief Concept: T is directly invocable with signature arguments and
/// returns void.
template <typename T, typename ArgsTuple>
concept DirectlyInvocableWith = []<typename... Args>(std::tuple<Args...>*) {
  using DecayedT = std::remove_cvref_t<T>;
  return std::invocable<DecayedT&, Args...> &&
         std::is_void_v<std::invoke_result_t<DecayedT&, Args...>>;
}(static_cast<ArgsTuple*>(nullptr));

/// @brief Concept: T is invocable with an index constant and signature
/// arguments, returning void.
template <typename T, size_t N, typename ArgsTuple>
concept IndexedInvocableWith = []<typename... Args>(std::tuple<Args...>*) {
  using DecayedT = std::remove_cvref_t<T>;
  return std::invocable<DecayedT&, std::integral_constant<size_t, N>,
                        Args...> &&
         std::is_void_v<std::invoke_result_t<
             DecayedT&, std::integral_constant<size_t, N>, Args...>>;
}(static_cast<ArgsTuple*>(nullptr));

/// @brief Concept: T is invocable with all signatures using indexed dispatch.
template <typename T, typename... Signatures>
concept AllIndexedInvocable = []<size_t... Indices>(
                                  std::index_sequence<Indices...>) {
  return (IndexedInvocableWith<T, Indices, SignatureArgsT<Signatures>> && ...);
}(std::make_index_sequence<sizeof...(Signatures)>{});

/// @brief Concept for callables usable with single-signature default Set.
template <typename T, typename Signature>
concept SingleSignatureCallable =
    DirectlyInvocableWith<T, SignatureArgsT<Signature>>;

/// @brief Concept for callables usable with multi-signature default Set.
template <typename T, typename... Signatures>
concept MultiSignatureCallable =
    (sizeof...(Signatures) > 1) && AllIndexedInvocable<T, Signatures...>;

/// @brief Helper struct for DefaultOpCallable concept with variadic
/// signatures.
template <typename T, typename... Sigs>
struct DefaultOpCallableHelper;

template <typename T, typename FirstSig>
struct DefaultOpCallableHelper<T, FirstSig> {
  static constexpr bool kValue = SingleSignatureCallable<T, FirstSig>;
};

template <typename T, typename FirstSig, typename... RestSigs>
struct DefaultOpCallableHelper<T, FirstSig, RestSigs...> {
  static constexpr bool kValue =
      MultiSignatureCallable<T, FirstSig, RestSigs...>;
};

/// @brief Concept for default Push callables.
template <typename T, typename... Sigs>
concept DefaultOpCallable =
    (sizeof...(Sigs) > 0) && DefaultOpCallableHelper<T, Sigs...>::kValue;

/// @brief Helper to check if a method is invocable on T with signature
/// arguments.
template <auto Method, typename T, typename ArgsTuple>
struct MethodInvocableWithHelper : std::false_type {};

template <auto Method, typename T, typename... Args>
  requires std::invocable<decltype(Method), T&, Args...> &&
           std::is_void_v<std::invoke_result_t<decltype(Method), T&, Args...>>
struct MethodInvocableWithHelper<Method, T, std::tuple<Args...>>
    : std::true_type {};

template <auto Method, typename T, typename... Args>
  requires std::invocable<decltype(Method), Args...> &&
           (!std::invocable<decltype(Method), T&, Args...>) &&
           std::is_void_v<std::invoke_result_t<decltype(Method), Args...>>
struct MethodInvocableWithHelper<Method, T, std::tuple<Args...>>
    : std::true_type {};

/// @brief Concept: a method is invocable on T with signature arguments and
/// returns void.
template <auto Method, typename T, typename ArgsTuple>
concept MethodInvocableWith =
    MethodInvocableWithHelper<Method, T, ArgsTuple>::value;

/// @brief Helper to validate all methods match their corresponding signatures.
template <typename T, typename SignatureTuple, auto... Methods>
struct AllMethodsValidHelper;

template <typename T, typename... Signatures, auto... Methods>
struct AllMethodsValidHelper<T, std::tuple<Signatures...>, Methods...> {
  static constexpr bool kValue = []<size_t... Indices>(
                                     std::index_sequence<Indices...>) {
    return (MethodInvocableWith<kNthMethod<Indices, Methods...>, T,
                                NthSignatureArgsT<Indices, Signatures...>> &&
            ...);
  }(std::make_index_sequence<sizeof...(Methods)>{});
};

/// @brief Concept: all methods are valid for their corresponding signatures.
template <typename T, typename SignatureTuple, auto... Methods>
concept AllMethodsValid =
    AllMethodsValidHelper<T, SignatureTuple, Methods...>::kValue;

/// @brief Concept for instantiated allocator types.
template <typename T>
concept InstantiatedAllocator =
    std::is_class_v<T> && !VoidSignature<T> && requires {
      typename std::allocator_traits<T>::template rebind_alloc<std::byte>;
    };

}  // namespace details

/**
 * @brief Concept for types storable in CallableBuffer.
 * @details Types must be destructible, copy or move constructible, not
 * references, not polymorphic, not abstract, and have alignment not exceeding
 * `std::max_align_t`.
 */
template <typename T>
concept CallableBufferStorable =
    std::destructible<T> &&
    (std::move_constructible<T> || std::copy_constructible<T>) &&
    !std::is_polymorphic_v<T> && !std::is_abstract_v<T> &&
    (alignof(std::remove_cvref_t<T>) <= alignof(std::max_align_t));

}  // namespace helios::container
