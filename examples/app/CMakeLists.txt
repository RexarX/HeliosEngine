if(HELIOS_BUILD_HELIOS_APP)
  add_subdirectory(dynamic_plugin)

  helios_add_example(
      NAME async_sub_app
      SOURCES async_sub_app.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME frame_limit
      SOURCES frame_limit.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME overlapping_sub_app
      SOURCES overlapping_sub_app.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME plugin
      SOURCES plugin.cpp
      MODULES helios_app helios_ecs helios_log
  )

  helios_add_example(
      NAME plugin_group
      SOURCES plugin_group.cpp
      MODULES helios_app helios_log
  )

  helios_add_example(
      NAME runner
      SOURCES runner.cpp
      MODULES helios_app helios_log
  )

  helios_add_example(
      NAME sub_app
      SOURCES sub_app.cpp
      MODULES helios_app helios_ecs helios_log
  )
endif()
