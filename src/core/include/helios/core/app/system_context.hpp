#pragma once

#include <helios/core_pch.hpp>

#include <helios/core/app/details/system_info.hpp>
#include <helios/core/assert.hpp>
#include <helios/core/async/executor.hpp>
#include <helios/core/async/sub_task_graph.hpp>
#include <helios/core/ecs/component.hpp>
#include <helios/core/ecs/details/system_local_storage.hpp>
#include <helios/core/ecs/entity.hpp>
#include <helios/core/ecs/entity_command_buffer.hpp>
#include <helios/core/ecs/event.hpp>
#include <helios/core/ecs/event_reader.hpp>
#include <helios/core/ecs/query.hpp>
#include <helios/core/ecs/resource.hpp>
#include <helios/core/ecs/world.hpp>
#include <helios/core/ecs/world_command_buffer.hpp>
#include <helios/core/logger.hpp>
#include <helios/core/memory/allocator_traits.hpp>
#include <helios/core/memory/frame_allocator.hpp>
#include <helios/core/memory/stl_allocator_adapter.hpp>
#include <helios/core/utils/common_traits.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <ranges>
#include <string_view>
#include <variant>

namespace helios::app {

/**
 * @brief Per-system execution context with validated access.
 * @details SystemContext wraps ecs::World to provide:
 * - Validated access to components and resources based on AccessPolicy
 * - Thread-safe query creation
 * - Local command buffer access for deferred operations
 * - Event emission capabilities
 * - Async execution support (Executor or SubTaskGraph)
 * - Frame allocator access for temporary allocations
 *
 * SystemContext is created once per system per update and holds only references.
 * All access is validated against the system's declared AccessPolicy.
 *
 * Query and command buffer methods use the frame allocator by default for
 * efficient temporary storage within systems.
 *
 * @note Pratially thread-safe.
 */
class SystemContext {
public:
  /// Allocator type for component type IDs in queries
  using QueryAllocator = memory::STLGrowableAllocator<ecs::ComponentTypeId, memory::FrameAllocator>;

  /// Allocator type for command pointers in command buffers
  using CommandAllocator = memory::STLGrowableAllocator<std::unique_ptr<ecs::Command>, memory::FrameAllocator>;

  /**
   * @brief Constructs a system context with Executor for main schedule systems.
   * @param world Reference to the ECS world
   * @param system_info Reference to system information
   * @param executor Reference to async executor
   * @param local_storage Reference to system local storage
   */
  SystemContext(ecs::World& world, const details::SystemInfo& system_info, async::Executor& executor,
                ecs::details::SystemLocalStorage& local_storage) noexcept
      : world_(world), system_info_(system_info), async_context_(std::ref(executor)), local_storage_(local_storage) {}

  /**
   * @brief Constructs a system context with SubTaskGraph for parallel schedule systems.
   * @param world Reference to the ECS world
   * @param system_info Reference to system information
   * @param sub_graph Reference to async sub task graph
   * @param local_storage Reference to system local storage
   */
  SystemContext(ecs::World& world, const details::SystemInfo& system_info, async::SubTaskGraph& sub_graph,
                ecs::details::SystemLocalStorage& local_storage) noexcept
      : world_(world), system_info_(system_info), async_context_(std::ref(sub_graph)), local_storage_(local_storage) {}

  SystemContext(const SystemContext&) = delete;
  SystemContext(SystemContext&&) = delete;
  ~SystemContext() noexcept = default;

  SystemContext& operator=(const SystemContext&) = delete;
  SystemContext& operator=(SystemContext&&) = delete;

  /**
   * @brief Creates a query builder for component queries (mutable access).
   * @details Creates a QueryBuilder that can be used to construct queries.
   * Runtime validation ensures queried components were declared in AccessPolicy.
   * Allows both const and mutable component access.
   * Uses the frame allocator for internal storage.
   * @note Thread-safe.
   * @return QueryBuilder for the world using frame allocator
   *
   * @example
   * @code
   * void MySystem(app::SystemContext& ctx) {
   *   auto query = ctx.Query().Get<Position&, const Velocity&>();
   *   query.ForEach([](Position& pos, const Velocity& vel) {
   *     pos.x += vel.dx;
   *   });
   * }
   * @endcode
   */
  [[nodiscard]] auto Query() noexcept -> ecs::QueryBuilder<QueryAllocator> {
    return {world_, system_info_.access_policy, QueryAllocator(local_storage_.FrameAllocator())};
  }

  /**
   * @brief Creates a read-only query builder for component queries.
   * @details Creates a QueryBuilder with const World for read-only operations.
   * Runtime validation ensures queried components were declared in AccessPolicy.
   * Only allows const component access - mutable access will fail at compile time.
   * Uses the frame allocator for internal storage.
   * @note Thread-safe.
   * @return ReadOnlyQueryBuilder for const world (read-only) using frame allocator
   *
   * @example
   * @code
   * void ReadOnlySystem(app::SystemContext& ctx) {
   *   auto query = ctx.ReadOnlyQuery().Get<const Position&>();  // OK - const component
   *   // auto query = ctx.ReadOnlyQuery().Get<Position&>();     // ERROR - mutable access from const World
   * }
   * @endcode
   */
  [[nodiscard]] auto ReadOnlyQuery() const noexcept -> ecs::ReadOnlyQueryBuilder<QueryAllocator> {
    return {world_, system_info_.access_policy, QueryAllocator(local_storage_.FrameAllocator())};
  }

  /**
   * @brief Creates a command buffer for deferred world operations.
   * @details Command buffer allows queuing operations that will be executed during ecs::World::Update().
   * This is the thread-safe way to modify the world.
   * Uses the frame allocator for internal storage.
   * @note Thread-safe, but keep in mind that world command buffer is not, avoid sharing between threads.
   * @return WorldCmdBuffer for queuing commands using frame allocator
   */
  [[nodiscard]] auto Commands() noexcept -> ecs::WorldCmdBuffer<CommandAllocator> {
    return ecs::WorldCmdBuffer<CommandAllocator>(local_storage_, CommandAllocator(local_storage_.FrameAllocator()));
  }

  /**
   * @brief Creates an entity command buffer for a specific entity.
   * @details Provides convenient interface for entity-specific operations.
   * Uses the frame allocator for internal storage.
   * @note Thread-safe, but keep in mind that entity command buffer is not, avoid sharing between threads.
   * @param entity Entity to operate on
   * @return EntityCmdBuffer for the entity using frame allocator
   */
  [[nodiscard]] auto EntityCommands(ecs::Entity entity) -> ecs::EntityCmdBuffer<CommandAllocator> {
    return {entity, local_storage_, CommandAllocator(local_storage_.FrameAllocator())};
  }

  /**
   * @brief Reserves an entity ID for deferred creation.
   * @details The actual entity creation is deferred until ecs::World::Update().
   * @note Thread-safe.
   * @return Reserved entity ID
   */
  [[nodiscard]] ecs::Entity ReserveEntity() { return world_.ReserveEntity(); }

  /**
   * @brief Gets mutable reference to a resource.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * - Resource not declared for write access.
   * - Resource doesn't exist.
   * @tparam T Resource type
   * @return Mutable reference to resource
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] T& WriteResource();

  /**
   * @brief Gets const reference to a resource.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * - Resource not declared for read access.
   * - Resource doesn't exist.
   * @tparam T Resource type
   * @return Const reference to resource
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] const T& ReadResource() const;

  /**
   * @brief Tries to get mutable pointer to a resource.
   * @note Thread-safe.
   * @warning Triggers assertion if resource not declared for write access.
   * @tparam T Resource type
   * @return Pointer to resource, or nullptr if not found
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] T* TryWriteResource();

  /**
   * @brief Tries to get const pointer to a resource.
   * @note Thread-safe.
   * @warning Triggers assertion if resource not declared for read access.
   * @tparam T Resource type
   * @return Const pointer to resource, or nullptr if not found
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] const T* TryReadResource() const;

  /**
   * @brief Emits an event to the local event queue.
   * @details Events are stored in system-local storage and flushed after schedule execution.
   * @note Not thread-safe, only one thread should emit events for a system at a time.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @param event Event to emit
   */
  template <ecs::EventTrait T>
  void EmitEvent(const T& event);

  /**
   * @brief Emits multiple events in bulk.
   * @details More efficient than calling EmitEvent multiple times.
   * @note Not thread-safe, only one thread should emit events for a system at a time.
   * @warning Triggers assertion if event type is not registered.
   * @tparam R Range of events
   * @param events Range of events to emit
   */
  template <std::ranges::sized_range R>
    requires ecs::EventTrait<std::ranges::range_value_t<R>>
  void EmitEventBulk(const R& events);

  /**
   * @brief Gets an event reader for type T.
   * @details Provides a type-safe, ergonomic API for reading events with
   * support for iteration, filtering, and searching.
   * Event must be registered via World::AddEvent<T>() first.
   * @note Thread-safe.
   * @warning Triggers assertion if event type is not registered.
   * @tparam T Event type
   * @return EventReader for type T
   */
  template <ecs::EventTrait T>
  [[nodiscard]] auto ReadEvents() const noexcept -> ecs::EventReader<T>;

  /**
   * @brief Checks if entity exists in the world.
   * @note Thread-safe.
   * @warning Triggers assertion if entity is invalid.
   * @param entity Entity to check.
   * @return True if entity exists, false otherwise.
   */
  [[nodiscard]] bool EntityExists(ecs::Entity entity) const;

  /**
   * @brief Checks if entity has component.
   * @note Thread-safe.
   * @warning Triggers assertion in next cases:
   * 1. Entity is invalid.
   * 2. World does not own entity.
   * @tparam T Component type.
   * @param entity Entity to check.
   * @return True if entity has component, false otherwise.
   */
  template <ecs::ComponentTrait T>
  [[nodiscard]] bool HasComponent(ecs::Entity entity) const;

  /**
   * @brief Checks if entity has components.
   * @note Thread-safe.
   * @warning Triggers assertion if entity is invalid.
   * @tparam Ts Components types.
   * @param entity Entity to check.
   * @return Array of bools indicating whether entity has each component (true if entity has component,
   * false otherwise).
   */
  template <ecs::ComponentTrait... Ts>
    requires utils::UniqueTypes<Ts...>
  [[nodiscard]] auto HasComponents(ecs::Entity entity) const -> std::array<bool, sizeof...(Ts)>;

  /**
   * @brief Checks if a resource exists.
   * @note Thread-safe.
   * @tparam T Resource type
   * @return True if resource exists, false otherwise
   */
  template <ecs::ResourceTrait T>
  [[nodiscard]] bool HasResource() const {
    return world_.HasResource<T>();
  }

  /**
   * @brief Checks if SubTaskGraph is available in this context.
   * @note Thread-safe.
   * @return True if sub task graph is available, false otherwise
   */
  [[nodiscard]] bool HasSubTaskGraph() const noexcept {
    return std::holds_alternative<std::reference_wrapper<async::SubTaskGraph>>(async_context_);
  }

  /**
   * @brief Checks if Executor is available in this context.
   * @note Thread-safe.
   * @return True if executor is available, false otherwise
   */
  [[nodiscard]] bool HasExecutor() const noexcept {
    return std::holds_alternative<std::reference_wrapper<async::Executor>>(async_context_);
  }

  /**
   * @brief Gets the number of entities in the world.
   * @note Thread-safe.
   * @return Number of entities in the world.
   */
  [[nodiscard]] size_t EntityCount() const noexcept { return world_.EntityCount(); }

  /**
   * @brief Gets frame allocator statistics.
   * @details Useful for debugging and profiling memory usage within systems.
   * @note Thread-safe.
   * @return Allocator statistics with current usage information
   */
  [[nodiscard]] memory::AllocatorStats FrameAllocatorStats() const noexcept {
    return local_storage_.FrameAllocatorStats();
  }

  /**
   * @brief Gets reference to the per-system frame allocator.
   * @details Use this allocator for temporary per-frame allocations that don't need individual deallocation.
   * The allocator is reset at frame boundaries.
   *
   * Ideal for:
   * - Temporary containers used within a single system execution
   * - Scratch buffers for algorithms
   * - Short-lived data structures
   *
   * @note Thread-safe.
   * @warning Data allocated with the frame allocator is only valid for the current frame.
   * All pointers and references to frame-allocated data become invalid after the frame ends.
   * Do not store frame-allocated data in components, resources, or any persistent storage.
   * @return Reference to the growable frame allocator
   */
  [[nodiscard]] ecs::details::SystemLocalStorage::FrameAllocatorType& FrameAllocator() noexcept {
    return local_storage_.FrameAllocator();
  }

  /**
   * @brief Gets const reference to the per-system frame allocator.
   * @note Thread-safe.
   * @warning Data allocated with the frame allocator is only valid for the current frame.
   * @return Const reference to the growable frame allocator
   */
  [[nodiscard]] const ecs::details::SystemLocalStorage::FrameAllocatorType& FrameAllocator() const noexcept {
    return local_storage_.FrameAllocator();
  }

  /**
   * @brief Creates an STL allocator adapter for the frame allocator.
   * @details Convenience method to create an STL-compatible allocator that uses the per-system frame allocator.
   * Use this with STL containers for zero-overhead temporary allocations.
   *
   * The frame allocator is reset at frame boundaries, making it ideal for temporary per-frame data.
   * Common use cases:
   * - Temporary containers for query results (via CollectWith())
   * - Temporary containers for event data (via CollectWith())
   * - Scratch buffers for algorithms
   * - Short-lived data structures that don't outlive the current frame
   *
   * @note Thread-safe.
   * @warning Data allocated with the frame allocator is only valid for the current frame.
   * All pointers and references to frame-allocated data become invalid after the frame ends.
   * Do not store frame-allocated data in components, resources, or any persistent storage.
   * @tparam T Type to allocate
   * @return STL allocator adapter for type T
   *
   * @example
   * @code
   * void MySystem::Update(app::SystemContext& ctx) {
   *   // Temporary container with frame allocator
   *   auto alloc = ctx.MakeFrameAllocator<int>();
   *   std::vector<int, decltype(alloc)> temp{alloc};
   *   temp.push_back(42);
   *   // Memory automatically reclaimed at frame end
   *
   *   // Use with query results
   *   auto query = ctx.Query().Get<Position&, Velocity&>();
   *   auto query_alloc = ctx.MakeFrameAllocator<decltype(query)::value_type>();
   *   auto results = query.CollectWith(query_alloc);
   *   // results valid only during this frame
   * }
   * @endcode
   */
  template <typename T>
  [[nodiscard]] auto MakeFrameAllocator() noexcept -> memory::STLGrowableAllocator<T, memory::FrameAllocator> {
    return memory::STLGrowableAllocator<T, memory::FrameAllocator>(local_storage_.FrameAllocator());
  }

  /**
   * @brief Gets reference to the sub task graph for parallel work.
   * @note Thread-safe.
   * @warning Triggers assertion if sub task graph is not available.
   * @return Reference to the async sub task graph
   */
  [[nodiscard]] async::SubTaskGraph& SubTaskGraph() noexcept;

  /**
   * @brief Gets reference to the executor for async work.
   * @note Thread-safe.
   * @warning Triggers assertion if executor is not available.
   * @return Reference to the executor
   */
  [[nodiscard]] async::Executor& Executor() noexcept;

  /**
   * @brief Gets the system information.
   * @note Thread-safe.
   * @return Const reference to system info
   */
  [[nodiscard]] const details::SystemInfo& GetSystemInfo() const noexcept { return system_info_; }

  /**
   * @brief Gets the system name.
   * @note Thread-safe.
   * @return System name
   */
  [[nodiscard]] std::string_view GetSystemName() const noexcept { return system_info_.name; }

private:
  /**
   * @brief Validates that a resource was declared for reading.
   * @tparam T Resource type
   */
  template <ecs::ResourceTrait T>
  void ValidateReadResource() const;

  /**
   * @brief Validates that a resource was declared for writing.
   * @tparam T Resource type
   */
  template <ecs::ResourceTrait T>
  void ValidateWriteResource() const;

  ecs::World& world_;                       ///< Reference to the ECS world
  const details::SystemInfo& system_info_;  ///< Reference to system information
  std::variant<std::reference_wrapper<async::Executor>, std::reference_wrapper<async::SubTaskGraph>>
      async_context_;                                ///< Async execution context (never null)
  ecs::details::SystemLocalStorage& local_storage_;  ///< Reference to system local storage
};

template <ecs::ResourceTrait T>
inline T& SystemContext::WriteResource() {
  ValidateWriteResource<T>();
  return world_.WriteResource<T>();
}

template <ecs::ResourceTrait T>
inline const T& SystemContext::ReadResource() const {
  ValidateReadResource<T>();
  return world_.ReadResource<T>();
}

template <ecs::ResourceTrait T>
inline T* SystemContext::TryWriteResource() {
  ValidateWriteResource<T>();
  return world_.TryWriteResource<T>();
}

template <ecs::ResourceTrait T>
inline const T* SystemContext::TryReadResource() const {
  ValidateReadResource<T>();
  return world_.TryReadResource<T>();
}

template <ecs::EventTrait T>
inline void SystemContext::EmitEvent(const T& event) {
  HELIOS_ASSERT(world_.HasEvent<T>(),
                "Failed to emit event of type '{}': Event type not registered in world! "
                "Add World::AddEvent<{}>() during initialization.",
                ecs::EventNameOf<T>(), ecs::EventNameOf<T>());
  local_storage_.WriteEvent(event);
}

template <std::ranges::sized_range R>
  requires ecs::EventTrait<std::ranges::range_value_t<R>>
inline void SystemContext::EmitEventBulk(const R& events) {
  using EventType = std::ranges::range_value_t<R>;
  HELIOS_ASSERT(world_.HasEvent<EventType>(),
                "Failed to emit events of type '{}': Event type not registered in world! "
                "Add World::AddEvent<{}>() during initialization.",
                ecs::EventNameOf<EventType>(), ecs::EventNameOf<EventType>());
  local_storage_.WriteEventBulk(events);
}

template <ecs::EventTrait T>
inline auto SystemContext::ReadEvents() const noexcept -> ecs::EventReader<T> {
  HELIOS_ASSERT(world_.HasEvent<T>(),
                "Failed to get event reader for type '{}': Event type not registered in world! "
                "Add World::AddEvent<{}>() during initialization.",
                ecs::EventNameOf<T>(), ecs::EventNameOf<T>());
  return world_.ReadEvents<T>();
}

inline bool SystemContext::EntityExists(ecs::Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity exists: Entity is invalid!");
  return world_.Exists(entity);
}

template <ecs::ComponentTrait T>
inline bool SystemContext::HasComponent(ecs::Entity entity) const {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity has component: Entity is invalid!");
  HELIOS_ASSERT(EntityExists(entity),
                "Failed to check if entity has component: World does not own entity with index '{}'!", entity.Index());
  return world_.HasComponent<T>(entity);
}

template <ecs::ComponentTrait... Ts>
  requires utils::UniqueTypes<Ts...>
inline auto SystemContext::HasComponents(ecs::Entity entity) const -> std::array<bool, sizeof...(Ts)> {
  HELIOS_ASSERT(entity.Valid(), "Failed to check if entity has components: Entity is invalid!");
  HELIOS_ASSERT(EntityExists(entity),
                "Failed to check if entity has components: World does not own entity with index '{}'!", entity.Index());
  return world_.HasComponents<Ts...>(entity);
}

inline async::SubTaskGraph& SystemContext::SubTaskGraph() noexcept {
  HELIOS_ASSERT(HasSubTaskGraph(),
                "Failed to get sub task graph: SubTaskGraph not available in this context! "
                "System '{}' is likely running on main schedule.",
                system_info_.name);
  return std::get<std::reference_wrapper<async::SubTaskGraph>>(async_context_).get();
}

inline async::Executor& SystemContext::Executor() noexcept {
  HELIOS_ASSERT(HasExecutor(),
                "Failed to get executor: Executor not available in this context! "
                "System '{}' is likely running on a parallel schedule.",
                system_info_.name);
  return std::get<std::reference_wrapper<async::Executor>>(async_context_).get();
}

template <ecs::ResourceTrait T>
inline void SystemContext::ValidateReadResource() const {
#ifdef HELIOS_ENABLE_ASSERTS
  if constexpr (ecs::IsResourceThreadSafe<T>()) {
    return;
  } else {
    const ecs::ResourceTypeId type_id = ecs::ResourceTypeIdOf<T>();
    const bool can_read =
        system_info_.access_policy.HasReadResource(type_id) || system_info_.access_policy.HasWriteResource(type_id);
    HELIOS_ASSERT(can_read,
                  "System '{}' attempted to read resource '{}' without declaring it in AccessPolicy! "
                  "Add .ReadResources<{}>() or .WriteResources<{}>() to {}::GetAccessPolicy().",
                  system_info_.name, ecs::ResourceNameOf<T>(), ecs::ResourceNameOf<T>(), ecs::ResourceNameOf<T>(),
                  system_info_.name);
  }
#endif
}

template <ecs::ResourceTrait T>
inline void SystemContext::ValidateWriteResource() const {
#ifdef HELIOS_ENABLE_ASSERTS
  if constexpr (ecs::IsResourceThreadSafe<T>()) {
    return;
  } else {
    const ecs::ResourceTypeId type_id = ecs::ResourceTypeIdOf<T>();
    const bool can_write = system_info_.access_policy.HasWriteResource(type_id);
    HELIOS_ASSERT(can_write,
                  "System '{}' attempted to write resource '{}' without declaring it in AccessPolicy! "
                  "Add .WriteResources<{}>() to {}::GetAccessPolicy().",
                  system_info_.name, ecs::ResourceNameOf<T>(), ecs::ResourceNameOf<T>(), system_info_.name);
  }
#endif
}

}  // namespace helios::app
