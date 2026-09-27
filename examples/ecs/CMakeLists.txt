if(HELIOS_BUILD_HELIOS_ECS)
  helios_add_example(
      NAME command
      SOURCES command.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME component
      SOURCES component.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME component_bundle
      SOURCES component_bundle.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME entity
      SOURCES entity.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME local_arena
      SOURCES local_arena.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
    NAME message
    SOURCES message.cpp
    MODULES helios_app helios_ecs helios_log
)

  helios_add_example(
      NAME query
      SOURCES query.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME resource
      SOURCES resource.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME schedule
      SOURCES schedule.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME stage
      SOURCES stage.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME system
      SOURCES system.cpp
      MODULES helios_app helios_ecs helios_log
  )
endif()
