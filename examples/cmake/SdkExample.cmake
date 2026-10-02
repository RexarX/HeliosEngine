include_guard(GLOBAL)

#[[
    Resolves the installed or in-tree Helios umbrella target for SDK examples.

    When built as part of Helios (add_subdirectory from examples/), links against
    the targets already defined in the current build. When configured standalone,
    uses find_package(Helios).
]]
function(helios_sdk_example_resolve_helios OUT_TARGET)
  if(TARGET helios::helios)
    set(${OUT_TARGET} helios::helios PARENT_SCOPE)
    return()
  endif()
  find_package(Helios 0.1 REQUIRED)
  set(${OUT_TARGET} helios::helios PARENT_SCOPE)
endfunction()

function(_helios_sdk_example_apply_conventions TARGET)
  if(COMMAND helios_apply_conventions)
    helios_apply_conventions(${TARGET})
  endif()
  if(COMMAND helios_target_set_output_dirs)
    helios_target_set_output_dirs(${TARGET} CUSTOM_FOLDER examples)
  endif()
  if(COMMAND helios_target_set_folder)
    helios_target_set_folder(${TARGET} "Examples/SDK")
  endif()
endfunction()

function(helios_add_c_sdk_example)
  helios_sdk_example_resolve_helios(_helios)
  add_executable(c_sdk_example main.c)
  target_link_libraries(c_sdk_example PRIVATE ${_helios})
  set_target_properties(c_sdk_example PROPERTIES
      C_STANDARD 11
      C_STANDARD_REQUIRED ON
      C_EXTENSIONS OFF
      LINKER_LANGUAGE CXX
  )
  _helios_sdk_example_apply_conventions(c_sdk_example)
endfunction()

function(helios_add_cpp_sdk_example)
  helios_sdk_example_resolve_helios(_helios)
  add_executable(cpp_sdk_example abi_check.cpp main.cpp)
  target_link_libraries(cpp_sdk_example PRIVATE ${_helios} helios::core)
  target_compile_features(cpp_sdk_example PRIVATE cxx_std_23)
  _helios_sdk_example_apply_conventions(cpp_sdk_example)
endfunction()
