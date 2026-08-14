# ============================================================================
# Sanitizer Configuration for Helios Engine
# ============================================================================
#
# Per-target sanitizer flags. Call helios_target_enable_sanitizers(<target>)
# (or helios_apply_conventions) on every TU that participates in a Debug
# sanitized link — including consumer game executables when embedding Helios
# with HELIOS_DEVELOPER_MODE=ON.
#
# For CPM / third-party targets under Helios, use
# helios_enable_sanitizers_in_binary_dir(<dir>) so MSVC ASan ABI stays consistent.
#
# Options (only when HELIOS_DEVELOPER_MODE=ON):
#   HELIOS_ENABLE_SANITIZERS       - Master switch (default: ON)
#   HELIOS_SANITIZER_ADDRESS       - ASan (default: ON)
#   HELIOS_SANITIZER_UNDEFINED     - UBSan (default: ON; not on MSVC)
#   HELIOS_SANITIZER_THREAD        - TSan (default: OFF; exclusive with ASan)
#   HELIOS_SANITIZER_MEMORY        - MSan (default: OFF; Clang only)
#
# ============================================================================

include_guard(GLOBAL)

if(NOT HELIOS_DEVELOPER_MODE)
  function(helios_target_enable_sanitizers TARGET)
  endfunction()

  function(helios_enable_sanitizers_in_binary_dir DIR)
  endfunction()

  function(helios_print_sanitizer_status)
    message(STATUS "Sanitizers: DISABLED (HELIOS_DEVELOPER_MODE=OFF)")
  endfunction()

  return()
endif()

set(HELIOS_COMPILER_IS_GNU OFF)
set(HELIOS_COMPILER_IS_CLANG OFF)
set(HELIOS_COMPILER_IS_MSVC OFF)
set(HELIOS_COMPILER_IS_CLANG_CL OFF)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(HELIOS_COMPILER_IS_GNU ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(HELIOS_COMPILER_IS_CLANG_CL ON)
  else()
    set(HELIOS_COMPILER_IS_CLANG ON)
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(HELIOS_COMPILER_IS_MSVC ON)
endif()

option(HELIOS_ENABLE_SANITIZERS "Enable sanitizers for Debug builds" ON)
option(HELIOS_SANITIZER_ADDRESS "Enable AddressSanitizer" ON)
option(HELIOS_SANITIZER_UNDEFINED "Enable UndefinedBehaviorSanitizer" ON)
option(HELIOS_SANITIZER_THREAD "Enable ThreadSanitizer (mutually exclusive with ASan)" OFF)
option(HELIOS_SANITIZER_MEMORY "Enable MemorySanitizer (Clang only, requires instrumented libc++)" OFF)

if(HELIOS_SANITIZER_ADDRESS AND HELIOS_SANITIZER_THREAD)
  message(WARNING "AddressSanitizer and ThreadSanitizer cannot be used together. Disabling ThreadSanitizer.")
  set(HELIOS_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
endif()

if(HELIOS_SANITIZER_ADDRESS AND HELIOS_SANITIZER_MEMORY)
  message(WARNING "AddressSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
  set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

if(HELIOS_SANITIZER_THREAD AND HELIOS_SANITIZER_MEMORY)
  message(WARNING "ThreadSanitizer and MemorySanitizer cannot be used together. Disabling MemorySanitizer.")
  set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

if(HELIOS_SANITIZER_MEMORY AND NOT HELIOS_COMPILER_IS_CLANG)
  message(WARNING "MemorySanitizer is only available with Clang. Disabling MemorySanitizer.")
  set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
endif()

if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
  if(HELIOS_SANITIZER_UNDEFINED)
    message(STATUS "UndefinedBehaviorSanitizer is not supported on MSVC. Disabling.")
    set(HELIOS_SANITIZER_UNDEFINED OFF CACHE BOOL "Enable UndefinedBehaviorSanitizer" FORCE)
  endif()
  if(HELIOS_SANITIZER_THREAD)
    message(STATUS "ThreadSanitizer is not supported on MSVC. Disabling.")
    set(HELIOS_SANITIZER_THREAD OFF CACHE BOOL "Enable ThreadSanitizer" FORCE)
  endif()
  if(HELIOS_SANITIZER_MEMORY)
    message(STATUS "MemorySanitizer is not supported on MSVC. Disabling.")
    set(HELIOS_SANITIZER_MEMORY OFF CACHE BOOL "Enable MemorySanitizer" FORCE)
  endif()
endif()

function(_helios_get_sanitizer_flags OUT_COMPILE_FLAGS OUT_LINK_FLAGS)
  set(_compile_flags "")
  set(_link_flags "")

  if(HELIOS_COMPILER_IS_MSVC AND HELIOS_SANITIZER_ADDRESS)
    set(_compile_flags "/fsanitize=address")
  elseif(HELIOS_COMPILER_IS_CLANG_CL AND HELIOS_SANITIZER_ADDRESS)
    set(_compile_flags "-fsanitize=address")
  elseif(HELIOS_COMPILER_IS_GNU OR HELIOS_COMPILER_IS_CLANG)
    set(_sanitizers "")
    if(HELIOS_SANITIZER_ADDRESS)
      list(APPEND _sanitizers "address")
    endif()
    if(HELIOS_SANITIZER_UNDEFINED)
      list(APPEND _sanitizers "undefined")
    endif()
    if(HELIOS_SANITIZER_THREAD)
      list(APPEND _sanitizers "thread")
    endif()
    if(HELIOS_SANITIZER_MEMORY)
      list(APPEND _sanitizers "memory")
    endif()

    if(_sanitizers)
      list(JOIN _sanitizers "," _sanitizer_list)
      set(_compile_flags "-fsanitize=${_sanitizer_list} -fno-omit-frame-pointer -fno-optimize-sibling-calls")
      set(_link_flags "-fsanitize=${_sanitizer_list}")
      if(HELIOS_SANITIZER_ADDRESS)
        string(APPEND _compile_flags " -fsanitize-address-use-after-scope")
      endif()
      if(HELIOS_SANITIZER_UNDEFINED)
        string(APPEND _compile_flags " -fno-sanitize-recover=undefined")
      endif()
    endif()
  endif()

  set(${OUT_COMPILE_FLAGS} "${_compile_flags}" PARENT_SCOPE)
  set(${OUT_LINK_FLAGS} "${_link_flags}" PARENT_SCOPE)
endfunction()

# Top-level builds: directory-scope under Helios so CPM/third-party share MSVC
# ASan ABI. Does not affect parent project targets when Helios is embedded
# (HELIOS_MANAGE_TOOLCHAIN defaults OFF when not top-level).
if(HELIOS_MANAGE_TOOLCHAIN AND HELIOS_ENABLE_SANITIZERS)
  _helios_get_sanitizer_flags(_helios_dir_sanitizer_compile _helios_dir_sanitizer_link)
  if(_helios_dir_sanitizer_compile)
    separate_arguments(_helios_dir_sanitizer_compile_list
        UNIX_COMMAND "${_helios_dir_sanitizer_compile}")
    foreach(_flag IN LISTS _helios_dir_sanitizer_compile_list)
      add_compile_options("$<$<CONFIG:Debug>:${_flag}>")
    endforeach()
  endif()
  if(_helios_dir_sanitizer_link)
    add_link_options("$<$<CONFIG:Debug>:SHELL:${_helios_dir_sanitizer_link}>")
  endif()
endif()

#[[
    helios_target_enable_sanitizers(<target>)

    Applies sanitizer compile/link options for Debug configs to a target.
    No-op when HELIOS_MANAGE_TOOLCHAIN already applied directory-scope flags
    (top-level Helios builds), except consumers outside the Helios tree still
    need this when embedding with developer mode.
]]
function(helios_target_enable_sanitizers TARGET)
  if(NOT HELIOS_ENABLE_SANITIZERS)
    return()
  endif()
  if(NOT TARGET ${TARGET})
    return()
  endif()

  get_target_property(_imported ${TARGET} IMPORTED)
  if(_imported)
    return()
  endif()

  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "UTILITY")
    return()
  endif()

  get_target_property(_already ${TARGET} HELIOS_SANITIZERS_APPLIED)
  if(_already)
    return()
  endif()

  # Directory-scope already covers targets created under the Helios tree when
  # managing the toolchain. Still apply per-target for consumers that call
  # helios_apply_conventions from outside Helios (embedding).
  if(HELIOS_MANAGE_TOOLCHAIN)
    # Mark applied so we do not double-attach; flags come from add_*_options.
    set_target_properties(${TARGET} PROPERTIES HELIOS_SANITIZERS_APPLIED TRUE)
    return()
  endif()

  _helios_get_sanitizer_flags(_compile_flags _link_flags)

  if(_compile_flags)
    separate_arguments(_compile_flags_list UNIX_COMMAND "${_compile_flags}")
    foreach(_flag IN LISTS _compile_flags_list)
      target_compile_options(${TARGET} PRIVATE "$<$<CONFIG:Debug>:${_flag}>")
    endforeach()
  endif()

  if(_link_flags)
    target_link_options(${TARGET} PRIVATE "$<$<CONFIG:Debug>:SHELL:${_link_flags}>")
  endif()

  set_target_properties(${TARGET} PROPERTIES HELIOS_SANITIZERS_APPLIED TRUE)
endfunction()

#[[
    helios_enable_sanitizers_in_binary_dir(<dir>)

    Enables sanitizers on all non-imported targets registered in a CMake binary
    directory. Used for CPM-fetched dependencies that must share MSVC ASan ABI.
]]
function(helios_enable_sanitizers_in_binary_dir DIR)
  if(NOT HELIOS_ENABLE_SANITIZERS)
    return()
  endif()
  if(NOT IS_DIRECTORY "${DIR}")
    return()
  endif()

  get_directory_property(_targets DIRECTORY "${DIR}" BUILDSYSTEM_TARGETS)
  if(NOT _targets)
    return()
  endif()

  foreach(_target IN LISTS _targets)
    if(TARGET ${_target})
      helios_target_enable_sanitizers(${_target})
    endif()
  endforeach()
endfunction()

function(helios_print_sanitizer_status)
  if(NOT HELIOS_ENABLE_SANITIZERS)
    message(STATUS "Sanitizers: DISABLED")
    return()
  endif()

  message(STATUS "")
  message(STATUS "========== Sanitizer Configuration ==========")
  message(STATUS "Sanitizers enabled for Debug builds")

  if(HELIOS_COMPILER_IS_MSVC OR HELIOS_COMPILER_IS_CLANG_CL)
    message(STATUS "  Compiler: MSVC/clang-cl (limited sanitizer support)")
    if(HELIOS_SANITIZER_ADDRESS)
      message(STATUS "  ✓ AddressSanitizer")
    endif()
  else()
    message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
    if(HELIOS_SANITIZER_ADDRESS)
      message(STATUS "  ✓ AddressSanitizer")
    endif()
    if(HELIOS_SANITIZER_UNDEFINED)
      message(STATUS "  ✓ UndefinedBehaviorSanitizer")
    endif()
    if(HELIOS_SANITIZER_THREAD)
      message(STATUS "  ✓ ThreadSanitizer")
    endif()
    if(HELIOS_SANITIZER_MEMORY)
      message(STATUS "  ✓ MemorySanitizer")
    endif()
  endif()

  message(STATUS "==============================================")
  message(STATUS "")
endfunction()
