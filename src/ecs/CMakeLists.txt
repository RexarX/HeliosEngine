option(HELIOS_ECS_ENABLE_PROFILE "Enable profiling in the ecs module if profile module is available" ON)

helios_module(
    NAME ecs
    VERSION 0.1.0
    DESCRIPTION "Entity Component System module"

    HEADERS
        include/helios/ecs/command/command.hpp
        include/helios/ecs/command/commands.hpp
        include/helios/ecs/command/entity_buffer.hpp
        include/helios/ecs/command/queue.hpp
        include/helios/ecs/command/world_buffer.hpp
        include/helios/ecs/component/archetype.hpp
        include/helios/ecs/component/archetype_id.hpp
        include/helios/ecs/component/component.hpp
        include/helios/ecs/component/manager.hpp
        include/helios/ecs/component/sparse_storage.hpp
        include/helios/ecs/details/profile.hpp
        include/helios/ecs/entity/entity.hpp
        include/helios/ecs/entity/manager.hpp
        include/helios/ecs/message/async_queue.hpp
        include/helios/ecs/message/async_reader.hpp
        include/helios/ecs/message/async_wrapper.hpp
        include/helios/ecs/message/async_writer.hpp
        include/helios/ecs/message/consumed_registry.hpp
        include/helios/ecs/message/manager.hpp
        include/helios/ecs/message/message.hpp
        include/helios/ecs/message/queue.hpp
        include/helios/ecs/message/reader.hpp
        include/helios/ecs/message/wrapper.hpp
        include/helios/ecs/message/writer.hpp
        include/helios/ecs/query/details/query_args.hpp
        include/helios/ecs/query/details/traits.hpp
        include/helios/ecs/query/iterator.hpp
        include/helios/ecs/query/query.hpp
        include/helios/ecs/resource/local_param.hpp
        include/helios/ecs/resource/manager.hpp
        include/helios/ecs/resource/param.hpp
        include/helios/ecs/resource/resource.hpp
        include/helios/ecs/system/access_decl.hpp
        include/helios/ecs/system/access_policy.hpp
        include/helios/ecs/system/composite_param.hpp
        include/helios/ecs/system/param_policy.hpp
        include/helios/ecs/system/param_traits.hpp
        include/helios/ecs/system/system.hpp
        include/helios/ecs/system/system_param.hpp
        include/helios/ecs/schedule/executor/executor.hpp
        include/helios/ecs/schedule/executor/main_thread.hpp
        include/helios/ecs/schedule/executor/multi_threaded.hpp
        include/helios/ecs/schedule/executor/single_threaded.hpp
        include/helios/ecs/schedule/dag.hpp
        include/helios/ecs/schedule/run_condition.hpp
        include/helios/ecs/schedule/schedule.hpp
        include/helios/ecs/schedule/scheduler.hpp
        include/helios/ecs/schedule/stage.hpp
        include/helios/ecs/schedule/system_local_data.hpp
        include/helios/ecs/schedule/system_set.hpp
        include/helios/ecs/schedule/system_storage.hpp
        include/helios/ecs/builtin_commands.hpp
        include/helios/ecs/builtin_messages.hpp
        include/helios/ecs/world.hpp
        include/helios/ecs/world_view.hpp

    SOURCES
        src/schedule/executor/main_thread.cpp
        src/schedule/executor/multi_threaded.cpp
        src/schedule/executor/single_threaded.cpp
        src/schedule/dag.cpp
        src/schedule/schedule.cpp
        src/schedule/scheduler.cpp

    PCH src/pch.hpp

    DEPENDS
        PUBLIC async
        PUBLIC compiler
        PUBLIC container
        PUBLIC core
        PUBLIC log
        PUBLIC memory
        PUBLIC utils

    OPTIONAL_DEPENDS
        profile

    USES
        concurrentqueue PUBLIC helios::concurrentqueue::concurrentqueue
        Boost PUBLIC helios::boost::unordered

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<BOOL:HELIOS_ECS_ENABLE_PROFILE>:HELIOS_ECS_ENABLE_PROFILE>

    TEST_SOURCES
        tests/command/command.cpp
        tests/command/commands.cpp
        tests/command/entity_buffer.cpp
        tests/command/queue.cpp
        tests/command/world_buffer.cpp
        tests/component/archetype.cpp
        tests/component/archetype_id.cpp
        tests/component/component.cpp
        tests/component/manager.cpp
        tests/component/sparse_storage.cpp
        tests/entity/entity.cpp
        tests/entity/manager.cpp
        tests/message/async_queue.cpp
        tests/message/async_reader.cpp
        tests/message/async_wrapper.cpp
        tests/message/async_writer.cpp
        tests/message/consumed_registry.cpp
        tests/message/manager.cpp
        tests/message/message.cpp
        tests/message/queue.cpp
        tests/message/reader.cpp
        tests/message/writer.cpp
        tests/query/details/traits.cpp
        tests/query/details/query_args.cpp
        tests/query/iterator.cpp
        tests/query/query.cpp
        tests/resource/local_param.cpp
        tests/resource/manager.cpp
        tests/resource/param.cpp
        tests/resource/resource.cpp
        tests/system/access_decl.cpp
        tests/system/access_policy.cpp
        tests/system/composite_param.cpp
        tests/system/param_policy.cpp
        tests/system/param_traits.cpp
        tests/system/system.cpp
        tests/schedule/dag.cpp
        tests/schedule/stage.cpp
        tests/schedule/run_condition.cpp
        tests/schedule/schedule.cpp
        tests/schedule/scheduler.cpp
        tests/schedule/system_local_data.cpp
        tests/schedule/system_set.cpp
        tests/schedule/system_storage.cpp
        tests/schedule/executor/main_thread.cpp
        tests/schedule/executor/multi_threaded.cpp
        tests/schedule/executor/single_threaded.cpp
        tests/builtin_commands.cpp
        tests/builtin_messages.cpp
        tests/main.cpp
        tests/world.cpp
        tests/world_view.cpp
)

include(SimdUtils)
helios_simd_apply_to_target(TARGET helios_module_ecs)
