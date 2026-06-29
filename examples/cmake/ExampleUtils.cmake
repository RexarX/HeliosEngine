include_guard(GLOBAL)

if(NOT COMMAND helios_target_set_cxx_standard)
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
        [SOURCES src/main.cpp ...]
        [HEADERS ...]
        [MODULES app log ...]
        [ENABLE_PROFILE]
        [DISABLE_SANITIZERS]
    )
]]
function(helios_add_example)
  cmake_parse_arguments(
    ARG
    "DISABLE_SANITIZERS;ENABLE_PROFILE"
    "NAME"
    "SOURCES;HEADERS;MODULES"
    ${ARGN}
  )

  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_add_example: NAME is required")
  endif()

  set(_target "${ARG_NAME}_example")

  if(NOT ARG_SOURCES)
    set(ARG_SOURCES src/main.cpp)
  endif()

  add_executable(${_target} ${ARG_HEADERS} ${ARG_SOURCES})

  helios_target_set_cxx_standard(${_target} STANDARD 23)
  helios_target_set_warnings(${_target})
  helios_target_set_platform(${_target})
  helios_target_set_optimization(${_target})
  helios_target_set_output_dirs(${_target} CUSTOM_FOLDER examples)
  helios_target_set_folder(${_target} "Examples")

  if(NOT ARG_DISABLE_SANITIZERS)
    helios_target_enable_sanitizers(${_target})
  endif()

  if(ARG_MODULES)
    helios_link_modules(TARGET ${_target} MODULES ${ARG_MODULES})
  endif()

  if(ARG_ENABLE_PROFILE)
    target_compile_definitions(${_target} PRIVATE
      $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:HELIOS_ENABLE_PROFILE>
    )
  endif()
endfunction()
