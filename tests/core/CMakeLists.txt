# Core module tests

# Unit test sources
set(CORE_UNIT_TESTS_SOURCES
    unit/assert.cpp
    unit/core.cpp
    unit/delegate.cpp
    unit/logger.cpp
    unit/main.cpp
    unit/random.cpp
    unit/timer.cpp
    unit/timestep.cpp
    unit/uuid.cpp

    unit/app/access_policy.cpp
    unit/app/app.cpp
    unit/app/dynamic_module.cpp
    unit/app/module.cpp
    unit/app/runners.cpp
    unit/app/schedule.cpp
    unit/app/sub_app.cpp
    unit/app/system_config.cpp
    unit/app/system_context.cpp
    unit/app/system_set.cpp
    unit/app/system_set_config.cpp
    unit/app/time.cpp
    unit/app/details/scheduler.cpp
    unit/app/details/system_diagnostics.cpp

    unit/async/async_task.cpp
    unit/async/common.cpp
    unit/async/executor.cpp
    unit/async/future.cpp
    unit/async/sub_task_graph.cpp
    unit/async/task.cpp
    unit/async/task_graph.cpp

    unit/container/sparse_set.cpp
    unit/container/static_string.cpp

    unit/memory/allocator_resources.cpp
    unit/memory/arena_allocator.cpp
    unit/memory/double_frame_allocator.cpp
    unit/memory/frame_allocator.cpp
    unit/memory/free_list_allocator.cpp
    unit/memory/growable_allocator.cpp
    unit/memory/n_frame_allocator.cpp
    unit/memory/pool_allocator.cpp
    unit/memory/stack_allocator.cpp
    unit/memory/stl_allocator_adapter.cpp

    unit/ecs/command.cpp
    unit/ecs/component.cpp
    unit/ecs/entity.cpp
    unit/ecs/entity_command_buffer.cpp
    unit/ecs/event.cpp
    unit/ecs/event_reader.cpp
    unit/ecs/event_writer.cpp
    unit/ecs/query.cpp
    unit/ecs/resource.cpp
    unit/ecs/world.cpp
    unit/ecs/world_command_buffer.cpp
    unit/ecs/details/archetype.cpp
    unit/ecs/details/command_buffer.cpp
    unit/ecs/details/command_queue.cpp
    unit/ecs/details/commands.cpp
    unit/ecs/details/components_manager.cpp
    unit/ecs/details/entities_manager.cpp
    unit/ecs/details/event_manager.cpp
    unit/ecs/details/event_queue.cpp
    unit/ecs/details/event_storage.cpp
    unit/ecs/details/query_cache.cpp
    unit/ecs/details/resources_manager.cpp
    unit/ecs/details/system_local_storage.cpp
    unit/ecs/events/builtin_events.cpp

    unit/utils/common_traits.cpp
    unit/utils/defer.cpp
    unit/utils/dynamic_library.cpp
    unit/utils/fast_pimpl.cpp
    unit/utils/filesystem.cpp
    unit/utils/functional_adapters.cpp
    unit/utils/string_hash.cpp
)

# Create unit test executable
helios_add_test(
    TARGET helios_core_unit
    MODULE_NAME core
    TYPE unit
    SOURCES ${CORE_UNIT_TESTS_SOURCES}
    DEPENDENCIES helios::core
)

# Integration test sources
set(CORE_INTEGRATION_TESTS_SOURCES
    integration/main.cpp
    integration/logger.cpp
    integration/app/app_integration.cpp
    integration/app/dynamic_module/dynamic_module.cpp
    integration/container/sparse_set.cpp
    integration/ecs/ecs_integration.cpp
    integration/memory/allocator_integration.cpp
)

# Build test module library for dynamic module tests
add_subdirectory(integration/app/dynamic_module)

# Create integration test executable
helios_add_test(
    TARGET helios_core_integration
    MODULE_NAME core
    TYPE integration
    SOURCES ${CORE_INTEGRATION_TESTS_SOURCES}
    DEPENDENCIES helios::core
)

# Ensure test module is built before integration tests run
add_dependencies(helios_core_integration helios_test_module)
