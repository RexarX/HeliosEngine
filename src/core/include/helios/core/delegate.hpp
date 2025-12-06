#pragma once

#include <helios/core_pch.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
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

/**
 * @brief Traits for free function pointers.
 */
template <typename FunctionSignature>
struct FreeFunctionTraits;

template <typename R, typename... Args>
struct FreeFunctionTraits<R (*)(Args...)> {
  using ReturnType = R;
  using Arguments = std::tuple<Args...>;
};

/**
 * @brief Traits for member function pointers (non-const and const).
 */
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

/**
 * @brief Concept to allow argument conversion with polymorphic relationships.
 * @details Allows regular convertibility or base/derived relationship for polymorphic types.
 * This is used to make delegates a bit more flexible when binding.
 */
template <typename From, typename To>
concept PolymorphicConvertible =
    std::convertible_to<From, To> ||
    (std::is_polymorphic_v<std::remove_reference_t<From>> && std::is_polymorphic_v<std::remove_reference_t<To>> &&
     (std::derived_from<From, To> || std::derived_from<To, From>));

}  // namespace details

/**
 * @brief Type-erased callable wrapper for free and member functions.
 * @details Delegate is a lightweight, non-owning wrapper for:
 * - Free functions
 * - Member functions (including const and virtual)
 *
 * It does not allocate and stores only:
 * - A raw instance pointer (for member functions, may be null for free functions)
 * - A function pointer to a small thunk that performs the actual call
 *
 * The delegate is intentionally minimal and exception-free.
 * It returns default-constructed values when empty (for non-void return types) and is a no-op for void return types.
 *
 * @tparam FunctionSignature Function type in the form `R(Args...)`.
 */
template <typename FunctionSignature>
class Delegate;

template <typename ReturnType, typename... Args>
class Delegate<ReturnType(Args...)> {
public:
  using FunctionType = ReturnType (*)(void*, Args...);

  /**
   * @brief Default constructs an empty delegate.
   */
  constexpr Delegate() noexcept = default;
  constexpr Delegate(const Delegate&) noexcept = default;
  constexpr Delegate(Delegate&&) noexcept = default;
  constexpr ~Delegate() noexcept = default;

  constexpr Delegate& operator=(const Delegate&) noexcept = default;
  constexpr Delegate& operator=(Delegate&&) noexcept = default;

  /**
   * @brief Reset delegate to empty state.
   */
  constexpr void Reset() noexcept;

  /**
   * @brief Invoke delegate with exact argument types.
   * @details If delegate is empty, returns default constructed ReturnType for non-void return types
   * and does nothing for void return type.
   * @warning If delegate is empty and ReturnType is not default constructible then using the returned value is UB.
   * @param args Arguments to forward to the bound callable.
   * @return Result of the invocation or default constructed value for empty delegate.
   */
  constexpr ReturnType Invoke(Args&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args&&...>);

  /**
   * @brief Invoke delegate with polymorphically convertible argument types.
   * @details This overload allows passing arguments that are convertible
   * or have base/derived relationship with the delegate's arguments.
   * @warning If delegate is empty and ReturnType is not default constructible then using the returned value is UB.
   * @tparam UArgs Parameter pack of actual argument types.
   * @param args Arguments to forward to the bound callable.
   * @return Result of the invocation or default constructed value for empty delegate.
   */
  template <typename... UArgs>
    requires(sizeof...(UArgs) == sizeof...(Args)) && (... && details::PolymorphicConvertible<UArgs, Args>)
  constexpr ReturnType Invoke(UArgs&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>);

  /**
   * @brief Create delegate from a free function pointer.
   * @details Binds a free function with no instance pointer required.
   * @tparam Func Free function pointer value.
   * @return Delegate bound to the given free function.
   */
  template <auto Func>
    requires(!std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::FreeFunctionTraits<decltype(Func)>::ReturnType> &&
             (sizeof...(Args) == std::tuple_size_v<typename details::FreeFunctionTraits<decltype(Func)>::Arguments>))
  static constexpr Delegate FromFunction() noexcept;

  /**
   * @brief Create delegate from a free function pointer with explicit signature type.
   * @details Variant that uses an explicit signature type instead of auto non-type template.
   * @tparam Signature Free function pointer type.
   * @tparam Func Free function pointer value.
   * @return Delegate bound to the given free function.
   */
  template <typename Signature, Signature Func>
    requires(!std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::FreeFunctionTraits<Signature>::ReturnType> &&
             (sizeof...(Args) == std::tuple_size_v<typename details::FreeFunctionTraits<Signature>::Arguments>))
  static constexpr Delegate FromFunction() noexcept;

  /**
   * @brief Create delegate from a member function pointer.
   * @details Binds a non-const or const member function pointer to a specific instance.
   * @warning Instance reference must remain valid for the lifetime of the delegate.
   * @tparam Func Member function pointer.
   * @param instance Reference to the object instance used for invocation.
   * @return Delegate bound to the given member function and instance.
   */
  template <auto Func>
    requires std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::MemberFunctionTraits<decltype(Func)>::ReturnType> &&
             (sizeof...(Args) == std::tuple_size_v<typename details::MemberFunctionTraits<decltype(Func)>::Arguments>)
  static constexpr Delegate FromFunction(
      typename details::MemberFunctionTraits<decltype(Func)>::Class& instance) noexcept;

  /**
   * @brief Create delegate from a member function pointer with explicit signature type.
   * @details Variant that uses an explicit signature type instead of auto non-type template.
   * @warning Instance reference must remain valid for the lifetime of the delegate.
   * @tparam Signature Member function pointer type.
   * @tparam Func Member function pointer value.
   * @param instance Reference to the object instance used for invocation.
   * @return Delegate bound to the given member function and instance.
   */
  template <typename Signature, Signature Func>
    requires std::is_member_function_pointer_v<decltype(Func)> &&
             std::same_as<ReturnType, typename details::MemberFunctionTraits<Signature>::ReturnType> &&
             (sizeof...(Args) == std::tuple_size_v<typename details::MemberFunctionTraits<Signature>::Arguments>)
  static constexpr Delegate FromFunction(typename details::MemberFunctionTraits<Signature>::Class& instance) noexcept;

  /**
   * @brief Invoke delegate with exact argument types.
   * @details Forwarding operator to Invoke.
   * @warning Same as for Invoke: empty delegate returns default constructed value for non-void return type.
   * @param args Arguments to forward to the bound callable.
   * @return Result of the invocation or default constructed value for empty delegate.
   */
  constexpr ReturnType operator()(Args&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args...>) {
    return Invoke(std::forward<Args>(args)...);
  }

  /**
   * @brief Invoke delegate with polymorphically convertible argument types.
   * @details Forwarding operator to Invoke for flexible argument passing.
   * @warning Same as for Invoke: empty delegate returns default constructed value for non-void return type.
   * @tparam UArgs Parameter pack of actual argument types.
   * @param args Arguments to forward to the bound callable.
   * @return Result of the invocation or default constructed value for empty delegate.
   */
  template <typename... UArgs>
    requires(sizeof...(UArgs) == sizeof...(Args)) && (... && details::PolymorphicConvertible<UArgs, Args>)
  constexpr ReturnType operator()(UArgs&&... args) const
      noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>) {
    return Invoke(std::forward<UArgs>(args)...);
  }

  /**
   * @brief Compare two delegates for equality.
   * @details Delegates are equal if they have the same instance and thunk pointer.
   * @param other Delegate to compare with.
   * @return True if equal, false otherwise.
   */
  constexpr bool operator==(const Delegate& other) const noexcept {
    return instance_ptr_ == other.instance_ptr_ && function_ptr_ == other.function_ptr_;
  }

  /**
   * @brief Compare two delegates for inequality.
   * @param other Delegate to compare with.
   * @return True if not equal, false otherwise.
   */
  constexpr bool operator!=(const Delegate& other) const noexcept { return !(*this == other); }

  /**
   * @brief Check if delegate is bound to a callable.
   * @return True if delegate is non-empty, false otherwise.
   */
  [[nodiscard]] constexpr bool Valid() const noexcept { return function_ptr_ != nullptr; }

  /**
   * @brief Get raw instance pointer stored inside delegate.
   * @details For free functions this will be nullptr.
   * For member functions this points to the bound object instance.
   * @return Raw instance pointer.
   */
  [[nodiscard]] constexpr void* InstancePtr() const noexcept { return instance_ptr_; }

private:
  void* instance_ptr_ = nullptr;
  FunctionType function_ptr_ = nullptr;
};

template <typename ReturnType, typename... Args>
constexpr void Delegate<ReturnType(Args...)>::Reset() noexcept {
  instance_ptr_ = nullptr;
  function_ptr_ = nullptr;
}

template <typename ReturnType, typename... Args>
constexpr auto Delegate<ReturnType(Args...)>::Invoke(Args&&... args) const
    noexcept(std::is_nothrow_invocable_v<FunctionType, void*, Args&&...>) -> ReturnType {
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
    return std::invoke(function_ptr_, instance_ptr_, std::forward<Args>(args)...);
  }
}

template <typename ReturnType, typename... Args>
template <typename... UArgs>
  requires(sizeof...(UArgs) == sizeof...(Args)) && (... && details::PolymorphicConvertible<UArgs, Args>)
constexpr auto Delegate<ReturnType(Args...)>::Invoke(UArgs&&... args) const
    noexcept(std::is_nothrow_invocable_v<FunctionType, void*, UArgs...>) -> ReturnType {
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
    return std::invoke(function_ptr_, instance_ptr_, std::forward<UArgs>(args)...);
  }
}

template <typename ReturnType, typename... Args>
template <auto Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::FreeFunctionTraits<decltype(Func)>::ReturnType> &&
           (sizeof...(Args) == std::tuple_size_v<typename details::FreeFunctionTraits<decltype(Func)>::Arguments>))
constexpr auto Delegate<ReturnType(Args...)>::FromFunction() noexcept -> Delegate {
  using Traits = details::FreeFunctionTraits<decltype(Func)>;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<std::size_t... I>(std::index_sequence<I...>) {
        return (... && details::PolymorphicConvertible<Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.function_ptr_ = [](void* /*instance*/, Args... call_args) noexcept(
                               std::is_nothrow_invocable_v<FunctionType, void*, Args...>) -> ReturnType {
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
      return;
    } else {
      return std::invoke(Func, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <typename Signature, Signature Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::FreeFunctionTraits<Signature>::ReturnType> &&
           (sizeof...(Args) == std::tuple_size_v<typename details::FreeFunctionTraits<Signature>::Arguments>))
constexpr auto Delegate<ReturnType(Args...)>::FromFunction() noexcept -> Delegate {
  using Traits = details::FreeFunctionTraits<Signature>;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<std::size_t... I>(std::index_sequence<I...>) {
        return (... && details::PolymorphicConvertible<Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.function_ptr_ = [](void* /*instance*/, Args... call_args) noexcept(
                               std::is_nothrow_invocable_v<FunctionType, void*, Args...>) -> ReturnType {
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
      return;
    } else {
      return std::invoke(Func, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <auto Func>
  requires std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::MemberFunctionTraits<decltype(Func)>::ReturnType> &&
           (sizeof...(Args) == std::tuple_size_v<typename details::MemberFunctionTraits<decltype(Func)>::Arguments>)
constexpr auto Delegate<ReturnType(Args...)>::FromFunction(
    typename details::MemberFunctionTraits<decltype(Func)>::Class& instance) noexcept -> Delegate {
  using Traits = details::MemberFunctionTraits<decltype(Func)>;
  using Class = typename Traits::Class;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<std::size_t... I>(std::index_sequence<I...>) {
        return (... && details::PolymorphicConvertible<Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.instance_ptr_ = &const_cast<std::remove_const_t<Class>&>(instance);
  delegate.function_ptr_ = [](void* instance_ptr, Args... call_args) noexcept(
                               std::is_nothrow_invocable_v<FunctionType, void*, Args...>) -> ReturnType {
    auto* typed_instance = static_cast<Class*>(instance_ptr);

    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, typed_instance, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
      return;
    } else {
      return std::invoke(Func, typed_instance, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
    }
  };

  return delegate;
}

template <typename ReturnType, typename... Args>
template <typename Signature, Signature Func>
  requires std::is_member_function_pointer_v<decltype(Func)> &&
           std::same_as<ReturnType, typename details::MemberFunctionTraits<Signature>::ReturnType> &&
           (sizeof...(Args) == std::tuple_size_v<typename details::MemberFunctionTraits<Signature>::Arguments>)
constexpr auto Delegate<ReturnType(Args...)>::FromFunction(
    typename details::MemberFunctionTraits<Signature>::Class& instance) noexcept -> Delegate {
  using Traits = details::MemberFunctionTraits<Signature>;
  using Class = typename Traits::Class;
  using Arguments = typename Traits::Arguments;

  static_assert(
      []<std::size_t... I>(std::index_sequence<I...>) {
        return (... && details::PolymorphicConvertible<Args, std::tuple_element_t<I, Arguments>>);
      }(std::make_index_sequence<sizeof...(Args)>{}),
      "Arguments must be convertible or have base-derived relationship");

  Delegate delegate;
  delegate.instance_ptr_ = &const_cast<std::remove_const_t<Class>&>(instance);
  delegate.function_ptr_ = [](void* instance_ptr, Args... call_args) noexcept(
                               std::is_nothrow_invocable_v<FunctionType, void*, Args...>) -> ReturnType {
    auto* typed_instance = static_cast<Class*>(instance_ptr);
    if constexpr (std::is_void_v<ReturnType>) {
      std::invoke(Func, typed_instance, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
      return;
    } else {
      return std::invoke(Func, typed_instance, static_cast<typename std::tuple_element_t<0, Arguments>>(call_args)...);
    }
  };

  return delegate;
}

/**
 * @brief Helper to create delegate from free function pointer.
 * @tparam Func Free function pointer
 * @return Delegate bound to the given free function
 */
template <auto Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)>)
constexpr auto DelegateFromFunction() noexcept {
  using Traits = details::FreeFunctionTraits<decltype(Func)>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<std::size_t... I>(std::index_sequence<I...>) {
    return Delegate<ReturnType(std::tuple_element_t<I, Args>...)>::template FromFunction<Func>();
  }(std::make_index_sequence<std::tuple_size_v<Args>>{});
}

/**
 * @brief Helper to create delegate from free function pointer with explicit signature.
 * @tparam Signature Free function pointer type
 * @tparam Func Free function pointer value
 * @return Delegate bound to the given free function
 */
template <typename Signature, Signature Func>
  requires(!std::is_member_function_pointer_v<decltype(Func)>)
constexpr auto DelegateFromFunction() noexcept {
  using Traits = details::FreeFunctionTraits<Signature>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<std::size_t... I>(std::index_sequence<I...>) {
    return Delegate<ReturnType(std::tuple_element_t<I, Args>...)>::template FromFunction<Signature, Func>();
  }(std::make_index_sequence<std::tuple_size_v<Args>>{});
}

/**
 * @brief Helper to create delegate from member function pointer.
 * @tparam Func Member function pointer
 * @param instance Reference to the object instance used for invocation
 * @return Delegate bound to the given member function and instance
 */
template <auto Func>
  requires std::is_member_function_pointer_v<decltype(Func)>
constexpr auto DelegateFromFunction(typename details::MemberFunctionTraits<decltype(Func)>::Class& instance) noexcept {
  using Traits = details::MemberFunctionTraits<decltype(Func)>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<std::size_t... I>(std::index_sequence<I...>, auto& inst) {
    return Delegate<ReturnType(std::tuple_element_t<I, Args>...)>::template FromFunction<Func>(inst);
  }(std::make_index_sequence<std::tuple_size_v<Args>>{}, instance);
}

/**
 * @brief Helper to create delegate from member function pointer with explicit signature.
 * @tparam Signature Member function pointer type
 * @tparam Func Member function pointer value
 * @param instance Reference to the object instance used for invocation
 * @return Delegate bound to the given member function and instance
 */
template <typename Signature, Signature Func>
  requires std::is_member_function_pointer_v<decltype(Func)>
constexpr auto DelegateFromFunction(typename details::MemberFunctionTraits<Signature>::Class& instance) noexcept {
  using Traits = details::MemberFunctionTraits<Signature>;
  using ReturnType = typename Traits::ReturnType;
  using Args = typename Traits::Arguments;

  return []<std::size_t... I>(std::index_sequence<I...>, auto& inst) {
    return Delegate<ReturnType(std::tuple_element_t<I, Args>...)>::template FromFunction<Signature, Func>(inst);
  }(std::make_index_sequence<std::tuple_size_v<Args>>{}, instance);
}

}  // namespace helios
