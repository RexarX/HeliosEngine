include(TargetUtils)
include(SimdUtils)
include(Sanitizers)

# Explicitly list all header files
set(CORE_HEADERS
    # Root headers
    include/helios/core_pch.hpp

    # Core headers
    include/helios/core/assert.hpp
    include/helios/core/core.hpp
    include/helios/core/delegate.hpp
    include/helios/core/logger.hpp
    include/helios/core/random.hpp
    include/helios/core/timer.hpp
    include/helios/core/timestep.hpp
    include/helios/core/uuid.hpp

    # App headers
    include/helios/core/app/access_policy.hpp
    include/helios/core/app/app.hpp
    include/helios/core/app/dynamic_module.hpp
    include/helios/core/app/module.hpp
    include/helios/core/app/runners.hpp
    include/helios/core/app/schedule.hpp
    include/helios/core/app/schedules.hpp
    include/helios/core/app/sub_app.hpp
    include/helios/core/app/system_config.hpp
    include/helios/core/app/system_context.hpp
    include/helios/core/app/system_set.hpp
    include/helios/core/app/system_set_config.hpp
    include/helios/core/app/time.hpp

    # App details headers
    include/helios/core/app/details/scheduler.hpp
    include/helios/core/app/details/system_diagnostics.hpp
    include/helios/core/app/details/system_info.hpp

    # Async headers
    include/helios/core/async/async_task.hpp
    include/helios/core/async/common.hpp
    include/helios/core/async/executor.hpp
    include/helios/core/async/future.hpp
    include/helios/core/async/sub_task_graph.hpp
    include/helios/core/async/task.hpp
    include/helios/core/async/task_graph.hpp

    # Container headers
    include/helios/core/container/static_string.hpp
    include/helios/core/container/sparse_set.hpp

    # ECS headers
    include/helios/core/ecs/command.hpp
    include/helios/core/ecs/component.hpp
    include/helios/core/ecs/entity.hpp
    include/helios/core/ecs/entity_command_buffer.hpp
    include/helios/core/ecs/event.hpp
    include/helios/core/ecs/event_reader.hpp
    include/helios/core/ecs/event_writer.hpp
    include/helios/core/ecs/query.hpp
    include/helios/core/ecs/resource.hpp
    include/helios/core/ecs/system.hpp
    include/helios/core/ecs/world.hpp
    include/helios/core/ecs/world_command_buffer.hpp

    # ECS details headers
    include/helios/core/ecs/details/archetype.hpp
    include/helios/core/ecs/details/command_buffer.hpp
    include/helios/core/ecs/details/command_queue.hpp
    include/helios/core/ecs/details/commands.hpp
    include/helios/core/ecs/details/components_manager.hpp
    include/helios/core/ecs/details/entities_manager.hpp
    include/helios/core/ecs/details/event_manager.hpp
    include/helios/core/ecs/details/event_queue.hpp
    include/helios/core/ecs/details/event_storage.hpp
    include/helios/core/ecs/details/query_cache.hpp
    include/helios/core/ecs/details/resources_manager.hpp
    include/helios/core/ecs/details/system_local_storage.hpp

    # ECS events headers
    include/helios/core/ecs/events/builtin_events.hpp

    # Memory headers
    include/helios/core/memory/allocator_resources.hpp
    include/helios/core/memory/allocator_traits.hpp
    include/helios/core/memory/arena_allocator.hpp
    include/helios/core/memory/common.hpp
    include/helios/core/memory/double_frame_allocator.hpp
    include/helios/core/memory/frame_allocator.hpp
    include/helios/core/memory/free_list_allocator.hpp
    include/helios/core/memory/growable_allocator.hpp
    include/helios/core/memory/n_frame_allocator.hpp
    include/helios/core/memory/pool_allocator.hpp
    include/helios/core/memory/stack_allocator.hpp
    include/helios/core/memory/stl_allocator_adapter.hpp

    # Utils headers
    include/helios/core/utils/common_traits.hpp
    include/helios/core/utils/defer.hpp
    include/helios/core/utils/dynamic_library.hpp
    include/helios/core/utils/fast_pimpl.hpp
    include/helios/core/utils/filesystem.hpp
    include/helios/core/utils/functional_adapters.hpp
    include/helios/core/utils/string_hash.hpp
)

# Explicitly list all source files
set(CORE_SOURCES
    # Root sources
    src/core_pch.cpp

    # Core sources
    src/core/assert.cpp
    src/core/logger.cpp

    # App sources
    src/core/app/app.cpp

    # Utils sources
    src/core/utils/dynamic_library.cpp

    # App details sources
    src/core/app/details/scheduler.cpp

    # ECS sources
    src/core/ecs/world.cpp

    # ECS details sources
    src/core/ecs/details/archetype.cpp
    src/core/ecs/details/entities_manager.cpp
    src/core/ecs/details/query_cache.cpp
)

# Create the library
add_library(helios_core STATIC ${CORE_HEADERS} ${CORE_SOURCES})
add_library(helios::core ALIAS helios_core)

# Apply standard configurations using modular helpers
helios_target_set_cxx_standard(helios_core STANDARD 23)
helios_target_set_platform(helios_core)
helios_target_set_optimization(helios_core)
helios_target_set_warnings(helios_core)
helios_target_set_output_dirs(helios_core)
helios_target_add_pch(helios_core ${CMAKE_CURRENT_SOURCE_DIR}/include/helios/core_pch.hpp)
helios_target_set_folder(helios_core "Helios/Modules")
helios_target_enable_sanitizers(helios_core)

# Enable Position Independent Code for linking into shared libraries
set_target_properties(helios_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Enable LTO if requested
if(HELIOS_ENABLE_LTO)
    helios_target_enable_lto(helios_core)
endif()

# Link dependencies (using new helios::*::* naming convention)
target_link_libraries(helios_core
    PUBLIC
        helios::boost::unordered
        helios::concurrentqueue::concurrentqueue
        helios::ctti::ctti
        helios::stduuid::stduuid
        helios::taskflow::taskflow
    PRIVATE
        helios::boost::stacktrace
        helios::spdlog::spdlog_header_only
)

# Link TBB on Linux with GCC - required for libstdc++ parallel STL backend
if(HELIOS_HAS_SYSTEM_TBB)
    target_link_libraries(helios_core PUBLIC helios::tbb::tbb)
    message(STATUS "helios_core: Linking TBB for libstdc++ parallel STL support")
endif()

# Include directories - configured per project structure
target_include_directories(helios_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    SYSTEM PRIVATE
        ${PROJECT_SOURCE_DIR}/third-party
)

# Apply SIMD optimizations
helios_simd_apply_to_target(TARGET helios_core)

# Unity build configuration
if(HELIOS_ENABLE_UNITY_BUILD)
    helios_target_enable_unity_build(helios_core
        BATCH_SIZE ${CMAKE_UNITY_BUILD_BATCH_SIZE}
        EXCLUDE_FILES
            include/helios/core_pch.hpp
            src/core_pch.cpp
    )
endif()

# Platform-specific configurations for stacktrace
if(HELIOS_USE_STL_STACKTRACE)
    # STL stacktrace is available, no Boost-specific configuration needed
    message(STATUS "helios_core: Using C++23 STL stacktrace")
else()
    # Using Boost stacktrace, configure platform-specific backends
    if(UNIX AND NOT APPLE)
        find_library(BACKTRACE_LIBRARY backtrace)
        if(BACKTRACE_LIBRARY)
            target_link_libraries(helios_core PRIVATE ${BACKTRACE_LIBRARY})
            target_compile_definitions(helios_core PRIVATE
                BOOST_STACKTRACE_USE_BACKTRACE
            )
        else()
            message(WARNING "libbacktrace not found, boost stacktrace will use basic backend")
        endif()
    elseif(APPLE)
        # macOS: _Unwind_Backtrace is available without _GNU_SOURCE
        target_compile_definitions(helios_core PRIVATE
            BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
        )
    elseif(WIN32)
        target_compile_definitions(helios_core PRIVATE
            BOOST_STACKTRACE_USE_WINDBG
        )
    endif()
endif()

# Build-specific definitions
target_compile_definitions(helios_core
    PRIVATE
        SPDLOG_USE_STD_FORMAT
        $<$<CONFIG:Release>:SPDLOG_NO_EXCEPTIONS>
    PUBLIC
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:HELIOS_ENABLE_ASSERTS HELIOS_ENABLE_STACKTRACE>
        $<$<BOOL:${HELIOS_ENABLE_PROFILING}>:HELIOS_ENABLE_PROFILING>
)
