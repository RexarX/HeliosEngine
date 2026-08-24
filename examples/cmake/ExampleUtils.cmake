include_guard(GLOBAL)

if(NOT COMMAND helios_apply_conventions)
  include(TargetUtils)
endif()
if(NOT COMMAND helios_link_modules)
  include(ModuleLinking)
endif()
if(NOT COMMAND helios_target_enable_sanitizers)
  include(Sanitizers)
endif()

#[[
    helios_add_example(
        NAME <example_name>           # target becomes <name>_example
        SOURCES src/main.cpp ...
        [HEADERS ...]
        [MODULES app ecs log ...]
        [DISABLE_SANITIZERS]
    )
]]
function(helios_add_example)
  cmake_parse_arguments(
    ARG
    "DISABLE_SANITIZERS"
    "NAME"
    "SOURCES;HEADERS;MODULES"
    ${ARGN}
  )

  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_add_example: NAME is required")
  endif()

  set(_target "${ARG_NAME}_example")

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "helios_add_example: SOURCES is required")
  endif()

  add_executable(${_target} ${ARG_HEADERS} ${ARG_SOURCES})

  if(ARG_DISABLE_SANITIZERS)
    helios_apply_conventions(${_target} NO_SANITIZERS)
  else()
    helios_apply_conventions(${_target})
  endif()

  helios_target_set_output_dirs(${_target} CUSTOM_FOLDER examples)
  helios_target_set_folder(${_target} "Examples")

  if(ARG_MODULES)
    helios_link_modules(TARGET ${_target} MODULES ${ARG_MODULES})
  endif()
endfunction()
