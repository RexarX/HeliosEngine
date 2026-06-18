#pragma once

#include <helios/ecs/component/component.hpp>
#include <helios/ecs/query/details/traits.hpp>
#include <helios/ecs/query/iterator.hpp>
#include <helios/utils/common_traits.hpp>

#include <array>
#include <tuple>
#include <type_traits>

namespace helios::ecs {

template <ComponentTrait... Ts>
struct With {};

template <ComponentTrait... Ts>
struct Without {};

namespace details {

template <typename T>
struct IsWithTag : std::false_type {};

template <ComponentTrait... Ts>
struct IsWithTag<With<Ts...>> : std::true_type {};

template <typename T>
struct IsWithoutTag : std::false_type {};

template <ComponentTrait... Ts>
struct IsWithoutTag<Without<Ts...>> : std::true_type {};

template <typename T>
inline constexpr bool kIsQueryFilter =
    IsWithTag<T>::value || IsWithoutTag<T>::value;

template <typename ComponentTuple, typename WithTuple, typename WithoutTuple>
struct QueryArgSplitResult;

template <typename... Cs, ComponentTrait... Ws, ComponentTrait... Wos>
struct QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                           std::tuple<Without<Wos...>>> {
  using Components = std::tuple<Cs...>;

  static constexpr std::array<ComponentTypeIndex, sizeof...(Ws)> kWithIndices =
      {ComponentTypeIndex::From<Ws>()...};

  static constexpr std::array<ComponentTypeIndex, sizeof...(Wos)>
      kWithoutIndices = {ComponentTypeIndex::From<Wos>()...};
};

template <typename Result, typename... Remaining>
struct QueryArgSplitHelper;

template <typename... Cs, ComponentTrait... Ws, ComponentTrait... Wos>
struct QueryArgSplitHelper<QueryArgSplitResult<
    std::tuple<Cs...>, std::tuple<With<Ws...>>, std::tuple<Without<Wos...>>>> {
  using type = QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                                   std::tuple<Without<Wos...>>>;
};

template <typename... Cs, ComponentTrait... Ws, ComponentTrait... Wos,
          ComponentTrait T, typename... Rest>
struct QueryArgSplitHelper<
    QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                        std::tuple<Without<Wos...>>>,
    T, Rest...> {
  using type = typename QueryArgSplitHelper<
      QueryArgSplitResult<std::tuple<Cs..., T>, std::tuple<With<Ws...>>,
                          std::tuple<Without<Wos...>>>,
      Rest...>::type;
};

template <typename... Cs, ComponentTrait... Ws, ComponentTrait... Wos,
          ComponentTrait... Ts, typename... Rest>
struct QueryArgSplitHelper<
    QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                        std::tuple<Without<Wos...>>>,
    With<Ts...>, Rest...> {
  using type = typename QueryArgSplitHelper<
      QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws..., Ts...>>,
                          std::tuple<Without<Wos...>>>,
      Rest...>::type;
};

template <typename... Cs, ComponentTrait... Ws, ComponentTrait... Wos,
          ComponentTrait... Ts, typename... Rest>
struct QueryArgSplitHelper<
    QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                        std::tuple<Without<Wos...>>>,
    Without<Ts...>, Rest...> {
  using type = typename QueryArgSplitHelper<
      QueryArgSplitResult<std::tuple<Cs...>, std::tuple<With<Ws...>>,
                          std::tuple<Without<Wos..., Ts...>>>,
      Rest...>::type;
};

template <typename WorldT, typename ComponentTuple>
struct QueryTypeInfo;

template <typename WorldT, typename... Cs>
struct QueryTypeInfo<WorldT, std::tuple<Cs...>> {
  static constexpr bool kAllConst = kAllComponentsConst<Cs...>;

  static constexpr bool kIsConst =
      std::is_const_v<std::remove_reference_t<WorldT>> || kAllConst;

  using ComponentManagerType =
      std::conditional_t<kIsConst, const ComponentManager, ComponentManager>;

  using ValueType = std::tuple<ComponentAccessType_t<Cs>...>;

  template <bool IsConst>
  using Iterator = BasicQueryIter<IsConst, Cs...>;

  template <bool IsConst>
  using WithEntityIterator = BasicQueryWithEntityIter<IsConst, Cs...>;
};

template <typename Tuple>
struct UniqueComponentAccessFromTuple;

template <typename... Cs>
struct UniqueComponentAccessFromTuple<std::tuple<Cs...>> {
  static constexpr bool kValue = UniqueComponentAccess<Cs...>;
};

template <typename WorldT, typename Tuple>
struct ValidWorldComponentAccessFromTuple;

template <typename WorldT, typename... Cs>
struct ValidWorldComponentAccessFromTuple<WorldT, std::tuple<Cs...>> {
  static constexpr bool kValue = ValidWorldComponentAccess<WorldT, Cs...>;
};

}  // namespace details

template <typename T>
concept QueryArg =
    details::ValidComponentAccess<T> || details::kIsQueryFilter<T>;

template <typename... Args>
concept QueryArgs = (QueryArg<Args> && ...) && (sizeof...(Args) > 0);

namespace details {

template <QueryArg... Args>
struct QueryArgSplit {
  using type = typename QueryArgSplitHelper<
      QueryArgSplitResult<std::tuple<>, std::tuple<With<>>,
                          std::tuple<Without<>>>,
      Args...>::type;

  using Components = typename type::Components;
  static constexpr auto kWithIndices = type::kWithIndices;
  static constexpr auto kWithoutIndices = type::kWithoutIndices;
};

}  // namespace details

}  // namespace helios::ecs
