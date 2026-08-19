#pragma once

#include <helios/utils/common_traits.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace helios {

namespace details {

template <typename T>
struct TupleToFunctionSignature;

template <typename R, typename... Args>
struct TupleToFunctionSignature<std::tuple<R, Args...>> {
  using Type = R(Args...);
};

template <typename FunctionSignature>
struct FreeFunctionTraits;

template <typename R, typename... Args>
struct FreeFunctionTraits<R (*)(Args...)> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

/// @brief Traits for member function pointers (non-const and const).
template <typename FunctionSignature>
struct MemberFunctionTraits;

template <typename C, typename R, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...)> {
  using Class = C;
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct MemberFunctionTraits<R (C::*)(Args...) const> {
  using Class = const C;
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

/// @brief Extracts ReturnType/Arguments from a pointer-to-member-function
/// type (used to unwrap `&Callable::operator()`).
template <typename MemberFnPtr>
struct MemberFnPtrTraits;

template <typename C, typename R, typename... Args>
struct MemberFnPtrTraits<R (C::*)(Args...)> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct MemberFnPtrTraits<R (C::*)(Args...) const> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct MemberFnPtrTraits<R (C::*)(Args...) noexcept> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

template <typename C, typename R, typename... Args>
struct MemberFnPtrTraits<R (C::*)(Args...) const noexcept> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

/// @brief True iff `&T::operator()` is well-formed — a single, non-overloaded,
/// non-template call operator.
template <typename T>
concept HasUnambiguousCallOperator = requires { &T::operator(); };

/**
 * @brief Traits for callables with a single `operator()` (lambdas, functors).
 * @details Overloaded or templated `operator()` yields no members so this can
 * be probed from a `requires` clause. Bind those callables with an explicit
 * `MakeDelegate<Signature>(callable)` instead.
 */
template <typename Callable>
struct CallableTraits {};

template <typename Callable>
  requires HasUnambiguousCallOperator<Callable>
struct CallableTraits<Callable>
    : MemberFnPtrTraits<decltype(&Callable::operator())> {};

/**
 * @brief True iff `T` decays to a function pointer via unary plus (`+t`).
 * @details Signature-agnostic: a capture-less lambda still qualifies when its
 * parameter or return types need an implicit conversion to match a Delegate
 * signature, rather than matching exactly.
 */
template <typename T>
concept DecaysToFunctionPointer = requires(T& callable) { +callable; };

/// @brief Empty, default-constructible class type (capture-less closures,
/// empty functors). Safe to reconstruct inside the thunk with no stored
/// pointer, so rvalue temporaries cannot dangle.
template <typename T>
concept EmptyDefaultCallable = std::is_class_v<T> && std::is_empty_v<T> &&
                               std::is_default_constructible_v<T>;

/// @brief Callable that can be bound from an rvalue without storing its
/// address.
template <typename T>
concept SafeTemporaryCallable =
    DecaysToFunctionPointer<T> || EmptyDefaultCallable<T>;

/// @brief Prepends `R` to an argument tuple for `TupleToFunctionSignature`.
template <typename R, typename ArgsTuple>
struct PrependReturnType;

template <typename R, typename... Args>
struct PrependReturnType<R, std::tuple<Args...>> {
  using Type = std::tuple<R, Args...>;
};

}  // namespace details

/**
 * @brief Type-erased, non-owning callable wrapper.
 * @details Delegate is a lightweight wrapper for:
 * - Free functions
 * - Member functions (including const and virtual)
 * - Capture-less lambdas and empty functors
 * - Stateful lambdas and functors (bound by address)
 *
 * It does not allocate and stores only:
 * - A raw instance pointer (may be null, or a decayed function pointer)
 * - A function pointer to a small thunk that performs the actual call
 *
 * Implicit converting constructors enable `std::function_ref`-style
 * parameters: a `Delegate<R(Args...)>` function argument can be passed a
 * matching callable directly.
 *
 * The delegate is intentionally minimal and exception-free.
 * It returns default-constructed values when empty (for non-void return types)
 * and is a no-op for void return types.
 *
 * @tparam FunctionSignature Function type in the form `R(Args...)`
 * @code
 * void take(Delegate<int(int)> cb);
 * take([](int x) { return x * 2; });
 *
 * int n = 3;
 * auto add = [n](int x) { return x + n; };
 * take(add);  // named capturing lambda: caller keeps it alive
 * @endcode
 */
template <typename FunctionSignature>
class Delegate;

template <typename ReturnType, typename... Args>
class Delegate<ReturnType(Args...)> {
public:
  using FunctionType = ReturnType (*)(void*, Args...);

  /// @brief Default constructs an empty delegate.
  constexpr Delegate() noexcept = default;

  /**
   * @brief Construct from a capture-less lambda, empty functor, or function
   * pointer.
   * @details Enabled when `Callable` is invocable with this signature and is
   * either empty and default-constructible or decays to a function pointer.
   * No pointer to the original object is stored, so temporaries are safe.
   * Argument and return conversions (including polymorphic ones) follow
   * ordinary call semantics.
   * @tparam Callable Stateless or empty callable type
   * @param callable Callable to bind
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<ReturnType, std::remove_cvref_t<Callable>&,
                                   Args...>)
  constexpr Delegate(Callable&& callable) noexcept {
    BindSafeTemporary(std::forward<Callable>(callable));
  }

  /**
   * @brief Construct from a named stateful callable (capturing lambda or
   * functor).
   * @details Stores a pointer to `callable` without taking ownership.
   * @warning `callable` must outlive this delegate.
   * @tparam Callable Stateful callable type
   * @param callable Lvalue callable to bind
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             !details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<
                 ReturnType, std::remove_reference_t<Callable>&, Args...> &&
             std::is_lvalue_reference_v<Callable>)
  constexpr Delegate(Callable&& callable) noexcept {
    BindStateful(std::forward<Callable>(callable));
  }

  /**
   * @brief Deleted: a stateful rvalue would leave a dangling pointer.
   * @details Name the callable (or pass a capture-less / empty one) instead.
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             !details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<
                 ReturnType, std::remove_reference_t<Callable>&, Args...> &&
             !std::is_lvalue_reference_v<Callable>)
  constexpr Delegate(Callable&&) noexcept = delete;

  constexpr Delegate(const Delegate&) noexcept = default;
  constexpr Delegate(Delegate&&) noexcept = default;
  constexpr ~Delegate() noexcept = default;

  constexpr Delegate& operator=(const Delegate&) noexcept = default;
  constexpr Delegate& operator=(Delegate&&) noexcept = default;

  /**
   * @brief Assign a capture-less lambda, empty functor, or function pointer.
   * @tparam Callable Stateless or empty callable type
   * @param callable Callable to bind
   * @return Reference to this delegate
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<ReturnType, std::remove_cvref_t<Callable>&,
                                   Args...>)
  constexpr Delegate& operator=(Callable&& callable) noexcept {
    BindSafeTemporary(std::forward<Callable>(callable));
    return *this;
  }

  /**
   * @brief Assign a named stateful callable.
   * @warning `callable` must outlive this delegate.
   * @tparam Callable Stateful callable type
   * @param callable Lvalue callable to bind
   * @return Reference to this delegate
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             !details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<ReturnType, Callable&, Args...>)
  constexpr Delegate& operator=(Callable& callable) noexcept {
    BindStateful(callable);
    return *this;
  }

  /**
   * @brief Deleted: assigning a stateful rvalue would dangle immediately.
   */
  template <typename Callable>
    requires(!std::same_as<std::remove_cvref_t<Callable>, Delegate> &&
             !details::SafeTemporaryCallable<std::remove_cvref_t<Callable>> &&
             std::is_invocable_r_v<
                 ReturnType, std::remove_reference_t<Callable>&, Args...> &&
             !std::is_lvalue_reference_v<Callable>)
  constexpr Delegate& operator=(Callable&&) noexcept = delete;

  /**
   * @brief Create delegate from a free function pointer.
   * @details Binds a free function with no instance pointer required.
   * @tparam Func Free function pointer value
   * @return Delegate bound to the given free function
   */
  template <auto Func>
    requires(
        !std::is_member_function_pointer_v<decltype(Func)> &&
        std::same_as<ReturnType, typename details::FreeFunctionTraits<
                                     decltype(Func)>::ReturnType> &&
        (sizeof...(Args) ==
         std::tuple_size_v<
             typename details::FreeFunctionTraits<decltype(Func)>::Arguments>))
  static constexpr Delegate From() noexcept;

  /**
   * @brief Create delegate from a free function pointer with explicit signature
   * type.
   * @details Variant that uses an explicit signature type instead of auto
   * non-type template.
   * @tparam Signature Free function pointer type
   * @tparam Func Free function pointer value
   * @return Delegate bound to the given free function
   */
  template <typename Signature, Signature Func>
    requires(!std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::FreeFunctionTraits<
                                          Signature>::ReturnType> &&
             (sizeof...(Args) ==
              std::tuple_size_v<
                  typename details::FreeFunctionTraits<Signature>::Arguments>))
  static constexpr Delegate From() noexcept;

  /**
   * @brief Create delegate from a member function pointer.
   * @details Binds a non-const or const member function pointer to a specific
   * instance.
   * @warning Instance reference must remain valid for the lifetime of the
   * delegate.
   * @tparam Func Member function pointer
   * @param instance Reference to the object instance used for invocation
   * @return Delegate bound to the given member function and instance
   */
  template <auto Func>
    requires std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::MemberFunctionTraits<
                                          decltype(Func)>::ReturnType> &&
             (sizeof...(Args) ==
              std::tuple_size_v<typename details::MemberFunctionTraits<
                  decltype(Func)>::Arguments>)
  static constexpr Delegate From(
      typename details::MemberFunctionTraits<decltype(Func)>::Class&
          instance) noexcept;

  /**
   * @brief Create delegate from a member function pointer with explicit
   * signature type.
   * @details Variant that uses an explicit signature type instead of auto
   * non-type template.
   * @warning Instance reference must remain valid for the lifetime of the
   * delegate.
   * @tparam Signature Member function pointer type
   * @tparam Func Member function pointer value
   * @param instance Reference to the object instance used for invocation
   * @return Delegate bound to the given member function and instance
   */
  template <typename Signature, Signature Func>
    requires std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::MemberFunctionTraits<
                                          Signature>::ReturnType> &&
             (sizeof...(Args) ==
              std::tuple_size_v<
                  typename details::MemberFunctionTraits<Signature>::Arguments>)
  static constexpr Delegate From(
      typename details::MemberFunctionTraits<Signature>::Class&
          instance) noexcept;

  /// @brief Reset delegate to empty state.
  constexpr void Reset() noexcept;

  /**
   * @brief Invoke delegate with exact argument types.
   * @details If delegate is empty, returns default constructed ReturnType for
   * non-void return types and does nothing for void return type.
   * @warning If delegate is empty and ReturnType is not default constructible
   * then using the returned value is UB.
   * @param args Arguments to forward to the bound callable
   * @return Result of the invocation or default constructed value for empty
   * delegate
   */
  constexpr ReturnType Invoke(Args&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args&&...>);

  /**
   * @brief Invoke delegate with polymorphically convertible argument types.
   * @details This overload allows passing arguments that are convertible
   * or have base/derived relationship with the delegate's arguments.
   * @warning If delegate is empty and ReturnType is not default constructible
   * then using the returned value is UB.
   * @tparam UArgs Parameter pack of actual argument types
   * @param args Arguments to forward to the bound callable
   * @return Result of the invocation or default constructed value for empty
   * delegate
   */
  template <typename... UArgs>
    requires(sizeof...(UArgs) == sizeof...(Args)) &&
            (... && utils::PolymorphicConvertible<UArgs, Args>)
  constexpr ReturnType Invoke(UArgs&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>);

  /**
   * @brief Invoke delegate with exact argument types.
   * @details Forwarding operator to Invoke.
   * @warning Same as for Invoke: empty delegate returns default constructed
   * value for non-void return type.
   * @param args Arguments to forward to the bound callable
   * @return Result of the invocation or default constructed value for empty
   * delegate
   */
  constexpr ReturnType operator()(Args&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args...>) {
    return Invoke(std::forward<Args>(args)...);
  }

  /**
   * @brief Invoke delegate with polymorphically convertible argument types.
   * @details Forwarding operator to Invoke for flexible argument passing.
   * @warning Same as for Invoke: empty delegate returns default constructed
   * value for non-void return type.
   * @tparam UArgs Parameter pack of actual argument types
   * @param args Arguments to forward to the bound callable
   * @return Result of the invocation or default constructed value for empty
   * delegate
   */
  template <typename... UArgs>
    requires(sizeof...(UArgs) == sizeof...(Args)) &&
            (... && utils::PolymorphicConvertible<UArgs, Args>)
  constexpr ReturnType operator()(UArgs&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>) {
    return Invoke(std::forward<UArgs>(args)...);
  }

  /**
   * @brief Compare two delegates for equality.
   * @details Delegates are equal if they have the same instance and thunk
   * pointer.
   * @param other Delegate to compare with
   * @return True if equal, false otherwise
   */
  constexpr bool operator==(const Delegate& other) const noexcept {
    return instance_ptr_ == other.instance_ptr_ &&
           function_ptr_ == other.function_ptr_;
  }

  /**
   * @brief Compare two delegates for inequality.
   * @param other Delegate to compare with
   * @return True if not equal, false otherwise
   */
  constexpr bool operator!=(const Delegate& other) const noexcept {
    return !(*this == other);
  }

  /**
   * @brief Check if delegate is bound to a callable.
   * @return True if delegate is non-empty, false otherwise
   */
  [[nodiscard]] constexpr bool Valid() const noexcept {
    return function_ptr_ != nullptr;
  }

  /**
   * @brief Get raw instance pointer stored inside delegate.
   * @details For `From` free functions and reconstructed empty callables this
   * is nullptr. For member functions and stateful callables this points to the
   * bound object. For function pointers bound via unary-plus decay, this holds
   * the function pointer value.
   * @return Raw instance pointer
   */
  [[nodiscard]] constexpr void* InstancePtr() const noexcept {
    return instance_ptr_;
  }

private:
  template <typename NativeFnPtr>
  void BindStateless(NativeFnPtr fn_ptr) noexcept;

  template <typename Callable>
  constexpr void BindEmpty() noexcept;

  template <typename Callable>
  constexpr void BindStateful(Callable&& callable) noexcept;

  template <typename Callable>
  constexpr void BindSafeTemporary(Callable&& callable) noexcept;

  void* instance_ptr_ = nullptr;
  FunctionType function_ptr_ = nullptr;
};

template <typename ReturnType, typename... Args>
template <auto Func>
  requires(
      !std::is_member_function_pointer_v<decltype(Func)> &&
      std::same_as<ReturnType, typename details::FreeFunctionTraits<
                                   decltype(Func)>::ReturnType> &&
      (sizeof...(Args) ==
       std::tuple_size_v<
           typename details::FreeFunctionTraits<decltype(Func)>::Arguments>))
constexpr auto Delegate<ReturnType(Args...)>::From() noexcept -> Delegate {
  using Traits = details::FreeFunctionTraits<decltype(Func)>;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<size_t... I>(std::index_sequence<I...>) {
        return (... && utils::PolymorphicConvertible<
                           Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.function_ptr_ =
      [](void* /*instance*/, Args... call_args) noexcept(
          std::is_nothrow_invocable_v<FunctionType, void*, Args...>)
      -> ReturnType {
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func,
                  static_cast<typename std::tuple_element_t<0, Arguments>>(
                      call_args)...);
      return;
    } else {
      return std::invoke(
          Func, static_cast<typename std::tuple_element_t<0, Arguments>>(
                    call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <typename Signature, Signature Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::FreeFunctionTraits<
                                        Signature>::ReturnType> &&
           (sizeof...(Args) ==
            std::tuple_size_v<
                typename details::FreeFunctionTraits<Signature>::Arguments>))
constexpr auto Delegate<ReturnType(Args...)>::From() noexcept -> Delegate {
  using Traits = details::FreeFunctionTraits<Signature>;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<size_t... I>(std::index_sequence<I...>) {
        return (... && utils::PolymorphicConvertible<
                           Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.function_ptr_ =
      [](void* /*instance*/, Args... call_args) noexcept(
          std::is_nothrow_invocable_v<FunctionType, void*, Args...>)
      -> ReturnType {
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func,
                  static_cast<typename std::tuple_element_t<0, Arguments>>(
                      call_args)...);
      return;
    } else {
      return std::invoke(
          Func, static_cast<typename std::tuple_element_t<0, Arguments>>(
                    call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <auto Func>
  requires std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::MemberFunctionTraits<
                                        decltype(Func)>::ReturnType> &&
           (sizeof...(Args) ==
            std::tuple_size_v<typename details::MemberFunctionTraits<
                decltype(Func)>::Arguments>)
constexpr auto Delegate<ReturnType(Args...)>::From(
    typename details::MemberFunctionTraits<decltype(Func)>::Class&
        instance) noexcept -> Delegate {
  using Traits = details::MemberFunctionTraits<decltype(Func)>;
  using Class = typename Traits::Class;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<size_t... I>(std::index_sequence<I...>) {
        return (... && utils::PolymorphicConvertible<
                           Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.instance_ptr_ = &const_cast<std::remove_const_t<Class>&>(instance);
  delegate.function_ptr_ =
      [](void* instance_ptr, Args... call_args) noexcept(
          std::is_nothrow_invocable_v<FunctionType, void*, Args...>)
      -> ReturnType {
    auto* typed_instance = static_cast<Class*>(instance_ptr);

    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, typed_instance,
                  static_cast<typename std::tuple_element_t<0, Arguments>>(
                      call_args)...);
      return;
    } else {
      return std::invoke(
          Func, typed_instance,
          static_cast<typename std::tuple_element_t<0, Arguments>>(
              call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <typename Signature, Signature Func>
  requires std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::MemberFunctionTraits<
                                        Signature>::ReturnType> &&
           (sizeof...(Args) ==
            std::tuple_size_v<
                typename details::MemberFunctionTraits<Signature>::Arguments>)
constexpr auto Delegate<ReturnType(Args...)>::From(
    typename details::MemberFunctionTraits<Signature>::Class& instance) noexcept
    -> Delegate {
  using Traits = details::MemberFunctionTraits<Signature>;
  using Class = typename Traits::Class;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<size_t... I>(std::index_sequence<I...>) {
        return (... && utils::PolymorphicConvertible<
                           Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.instance_ptr_ = &const_cast<std::remove_const_t<Class>&>(instance);
  delegate.function_ptr_ =
      [](void* instance_ptr, Args... call_args) noexcept(
          std::is_nothrow_invocable_v<FunctionType, void*, Args...>)
      -> ReturnType {
    auto* typed_instance = static_cast<Class*>(instance_ptr);
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, typed_instance,
                  static_cast<typename std::tuple_element_t<0, Arguments>>(
                      call_args)...);

      return;
    } else {
      return std::invoke(
          Func, typed_instance,
          static_cast<typename std::tuple_element_t<0, Arguments>>(
              call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
constexpr void Delegate<ReturnType(Args...)>::Reset() noexcept {
  instance_ptr_ = nullptr;
  function_ptr_ = nullptr;
}

template <typename ReturnType, typename... Args>
constexpr auto Delegate<ReturnType(Args...)>::Invoke(Args&&... args) const
    noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args&&...>)
        -> ReturnType {
  if (function_ptr_ == nullptr) [[unlikely]] {
    if constexpr (std::is_void_v<ReturnType>) {
      return;
    } else {
      return {};
    }
  }

  if constexpr (std::is_void_v<ReturnType>) {
    std::invoke(function_ptr_, instance_ptr_, std::forward<Args>(args)...);
    return;
  } else {
    return std::invoke(function_ptr_, instance_ptr_,
                       std::forward<Args>(args)...);
  }
}

template <typename ReturnType, typename... Args>
template <typename... UArgs>
  requires(sizeof...(UArgs) == sizeof...(Args)) &&
          (... && utils::PolymorphicConvertible<UArgs, Args>)
constexpr auto Delegate<ReturnType(Args...)>::Invoke(UArgs&&... args) const
    noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>)
        -> ReturnType {
  if (function_ptr_ == nullptr) [[unlikely]] {
    if constexpr (std::is_void_v<ReturnType>) {
      return;
    } else {
      return {};
    }
  }

  if constexpr (std::is_void_v<ReturnType>) {
    std::invoke(function_ptr_, instance_ptr_, std::forward<UArgs>(args)...);
    return;
  } else {
    return std::invoke(function_ptr_, instance_ptr_,
                       std::forward<UArgs>(args)...);
  }
}

template <typename ReturnType, typename... Args>
template <typename NativeFnPtr>
inline void Delegate<ReturnType(Args...)>::BindStateless(
    NativeFnPtr fn_ptr) noexcept {
  instance_ptr_ = reinterpret_cast<void*>(fn_ptr);
  function_ptr_ =
      [](void* instance_ptr, Args... call_args) noexcept(
          std::is_nothrow_invocable_v<NativeFnPtr, Args...>) -> ReturnType {
    auto fn = reinterpret_cast<NativeFnPtr>(instance_ptr);
    if constexpr (std::is_void_v<ReturnType>) {
      fn(call_args...);
      return;
    } else {
      return fn(call_args...);
    }
  };
}

template <typename ReturnType, typename... Args>
template <typename Callable>
constexpr void Delegate<ReturnType(Args...)>::BindEmpty() noexcept {
  using StoredType = std::remove_cvref_t<Callable>;
  instance_ptr_ = nullptr;
  function_ptr_ =
      [](void* /*instance_ptr*/, Args... call_args) noexcept(
          std::is_nothrow_invocable_r_v<ReturnType, StoredType, Args...>)
      -> ReturnType {
    StoredType callable{};
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(callable, call_args...);
      return;
    } else {
      return std::invoke(callable, call_args...);
    }
  };
}

template <typename ReturnType, typename... Args>
template <typename Callable>
constexpr void Delegate<ReturnType(Args...)>::BindStateful(
    Callable&& callable) noexcept {
  using StoredType = std::remove_reference_t<Callable>;
  instance_ptr_ =
      const_cast<void*>(static_cast<const void*>(std::addressof(callable)));
  function_ptr_ =
      [](void* instance_ptr, Args... call_args) noexcept(
          std::is_nothrow_invocable_r_v<ReturnType, StoredType&, Args...>)
      -> ReturnType {
    auto* typed_callable = static_cast<StoredType*>(instance_ptr);
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(*typed_callable, call_args...);
      return;
    } else {
      return std::invoke(*typed_callable, call_args...);
    }
  };
}

template <typename ReturnType, typename... Args>
template <typename Callable>
constexpr void Delegate<ReturnType(Args...)>::BindSafeTemporary(
    Callable&& callable) noexcept {
  using T = std::remove_cvref_t<Callable>;
  if constexpr (details::EmptyDefaultCallable<T>) {
    BindEmpty<T>();
  } else {
    BindStateless(+callable);
  }
}

/**
 * @brief Helper to create delegate from free function pointer.
 * @tparam Func Free function pointer.
 * @return Delegate bound to the given free function.
 */
template <auto Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)>)
constexpr auto MakeDelegate() noexcept {
  using Traits = details::FreeFunctionTraits<decltype(Func)>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<size_t... I>(std::index_sequence<I...>) {
    return Delegate<ReturnType(
        std::tuple_element_t<I, Args>...)>::template From<Func>();
  }(std::make_index_sequence<std::tuple_size_v<Args>>{});
}

/**
 * @brief Helper to create delegate from member function pointer.
 * @tparam Func Member function pointer.
 * @param instance Reference to the object instance used for invocation.
 * @return Delegate bound to the given member function and instance.
 */
template <auto Func>
  requires std::is_member_function_pointer_v<decltype(Func)>
constexpr auto MakeDelegate(
    typename details::MemberFunctionTraits<decltype(Func)>::Class&
        instance) noexcept {
  using Traits = details::MemberFunctionTraits<decltype(Func)>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<size_t... I>(std::index_sequence<I...>, auto& inst) {
    return Delegate<ReturnType(
        std::tuple_element_t<I, Args>...)>::template From<Func>(inst);
  }(std::make_index_sequence<std::tuple_size_v<Args>>{}, instance);
}

/**
 * @brief Create a delegate from a callable, deducing the signature from
 * `operator()`.
 * @details Stateless and empty callables may be temporaries. Stateful
 * callables must be lvalues; see `Delegate`'s constructors.
 * @note Unavailable when `operator()` is overloaded or templated — use
 * `MakeDelegate<Signature>(callable)` instead.
 * @tparam Callable Callable type (deduced)
 * @param callable Callable to bind
 * @return Delegate bound to the given callable
 */
template <typename Callable>
  requires details::HasUnambiguousCallOperator<std::remove_cvref_t<Callable>>
constexpr auto MakeDelegate(Callable&& callable) noexcept {
  using Traits = details::CallableTraits<std::remove_cvref_t<Callable>>;
  using ReturnType = typename Traits::ReturnType;
  using Arguments = typename Traits::Arguments;
  using Signature = typename details::TupleToFunctionSignature<
      typename details::PrependReturnType<ReturnType, Arguments>::Type>::Type;

  return Delegate<Signature>(std::forward<Callable>(callable));
}

/**
 * @brief Create a delegate from a callable with an explicit signature.
 * @details Use when `operator()` is overloaded or generic so no single
 * signature can be deduced.
 * @tparam Signature Function type in the form `R(Args...)`
 * @tparam Callable Callable type (deduced)
 * @param callable Callable to bind
 * @return Delegate bound to the given callable
 */
template <typename Signature, typename Callable>
constexpr auto MakeDelegate(Callable&& callable) noexcept {
  return Delegate<Signature>(std::forward<Callable>(callable));
}

}  // namespace helios
