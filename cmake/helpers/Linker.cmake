# Helios Engine fast-linker selection and RelWithDebInfo LTO mode
#
# Detects mold/lld/lld-link based on HELIOS_LINKER. Project-wide CMAKE_LINKER /
# CMAKE_AR mutation and directory-scope -fuse-ld apply only when
# HELIOS_MANAGE_TOOLCHAIN is ON (default: top-level builds). When embedded,
# Helios targets opt in via helios_target_apply_linker().
#
# RelWithDebInfo ThinLTO / parallel WHOPR / MSVC incremental LTCG is applied
# per-target by helios_target_apply_lto_mode() (called from helios_target_enable_lto).

include_guard(GLOBAL)

if(NOT DEFINED HELIOS_LINKER)
  if(PROJECT_IS_TOP_LEVEL)
    set(_helios_linker_default "AUTO")
  else()
    set(_helios_linker_default "DEFAULT")
  endif()
  set(HELIOS_LINKER "${_helios_linker_default}" CACHE STRING
      "Linker to use: AUTO, MOLD, LLD, DEFAULT")
endif()
set_property(CACHE HELIOS_LINKER PROPERTY STRINGS "AUTO" "MOLD" "LLD" "DEFAULT")

#[[
    helios_configure_linker()

    Detects a fast linker based on HELIOS_LINKER. When HELIOS_MANAGE_TOOLCHAIN
    is ON, applies it project-wide (CMAKE_LINKER / -fuse-ld). Otherwise only
    records HELIOS_ACTIVE_LINKER for helios_target_apply_linker().

    Sets HELIOS_ACTIVE_LINKER (CACHE INTERNAL) to one of:
        "mold", "lld", "lld-link", "default"
]]
function(helios_configure_linker)
  set(_cache_key "${HELIOS_LINKER}:${HELIOS_MANAGE_TOOLCHAIN}")
  if(DEFINED HELIOS_LINKER_CACHED AND HELIOS_LINKER_CACHE_KEY STREQUAL "${_cache_key}")
    return()
  endif()

  set(_chosen "default")

  if(HELIOS_LINKER STREQUAL "DEFAULT")
    set(HELIOS_ACTIVE_LINKER "default" CACHE INTERNAL "Active linker")
    set(HELIOS_LINKER_CACHED TRUE CACHE INTERNAL "Linker selection cached")
    set(HELIOS_LINKER_CACHE_KEY "${_cache_key}" CACHE INTERNAL "Linker selection cache key")
    message(STATUS "Linker: using the platform default (HELIOS_LINKER=DEFAULT)")
    return()
  endif()

  set(_msvc_abi FALSE)
  if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
      AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    set(_msvc_abi TRUE)
  endif()

  if(_msvc_abi)
    if(HELIOS_LINKER MATCHES "^(AUTO|LLD)$")
      find_program(HELIOS_LLD_LINK_EXECUTABLE
          NAMES lld-link lld-link.exe
          HINTS
              "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin"
              "$ENV{VCINSTALLDIR}/Tools/Llvm/bin"
              "C:/Program Files/LLVM/bin"
              "C:/Program Files (x86)/LLVM/bin"
      )
      if(HELIOS_LLD_LINK_EXECUTABLE)
        set(_chosen "lld-link")
      elseif(HELIOS_LINKER STREQUAL "LLD")
        message(WARNING "HELIOS_LINKER=LLD requested but lld-link was not found on PATH; "
            "falling back to the default MSVC linker")
      endif()
    elseif(HELIOS_LINKER STREQUAL "MOLD")
      message(WARNING "HELIOS_LINKER=MOLD is not supported on the MSVC ABI; "
          "falling back to the default MSVC linker")
    endif()
  else()
    set(_allow_mold TRUE)
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      set(_allow_mold FALSE)
    endif()

    if(_allow_mold AND HELIOS_LINKER MATCHES "^(AUTO|MOLD)$")
      find_program(HELIOS_MOLD_EXECUTABLE mold)
      if(HELIOS_MOLD_EXECUTABLE)
        set(_chosen "mold")
      endif()
    endif()

    if(_chosen STREQUAL "default" AND HELIOS_LINKER MATCHES "^(AUTO|LLD)$")
      find_program(HELIOS_LLD_EXECUTABLE ld.lld)
      if(HELIOS_LLD_EXECUTABLE)
        set(_chosen "lld")
      endif()
    endif()

    if(_chosen STREQUAL "default")
      if(HELIOS_LINKER STREQUAL "MOLD")
        if(NOT _allow_mold)
          message(WARNING "HELIOS_LINKER=MOLD requested but mold cannot link Mach-O; "
              "falling back to the default linker")
        else()
          message(WARNING "HELIOS_LINKER=MOLD requested but mold was not found on PATH; "
              "falling back to the default linker")
        endif()
      elseif(HELIOS_LINKER STREQUAL "LLD")
        message(WARNING "HELIOS_LINKER=LLD requested but ld.lld was not found on PATH; "
            "falling back to the default linker")
      endif()
    endif()
  endif()

  set(HELIOS_ACTIVE_LINKER "${_chosen}" CACHE INTERNAL "Active linker")
  set(HELIOS_LINKER_CACHED TRUE CACHE INTERNAL "Linker selection cached")
  set(HELIOS_LINKER_CACHE_KEY "${_cache_key}" CACHE INTERNAL "Linker selection cache key")

  if(HELIOS_MANAGE_TOOLCHAIN AND NOT _chosen STREQUAL "default")
    if(_msvc_abi AND _chosen STREQUAL "lld-link")
      set(CMAKE_LINKER "${HELIOS_LLD_LINK_EXECUTABLE}" CACHE FILEPATH "Linker" FORCE)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
        set(CMAKE_LINKER_TYPE "LLD")
      endif()
    elseif(NOT _msvc_abi)
      if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
        if(_chosen STREQUAL "mold")
          set(CMAKE_LINKER_TYPE "MOLD")
        elseif(_chosen STREQUAL "lld")
          set(CMAKE_LINKER_TYPE "LLD")
        endif()
      endif()
      add_link_options(
          $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>:-fuse-ld=${_chosen}>
      )
    endif()
    message(STATUS "Linker: using '${_chosen}' project-wide (HELIOS_LINKER=${HELIOS_LINKER}, HELIOS_MANAGE_TOOLCHAIN=ON)")
  elseif(NOT _chosen STREQUAL "default")
    message(STATUS "Linker: '${_chosen}' available for Helios targets (HELIOS_LINKER=${HELIOS_LINKER}, HELIOS_MANAGE_TOOLCHAIN=OFF)")
  else()
    message(STATUS "Linker: no fast linker found, using the platform default (HELIOS_LINKER=${HELIOS_LINKER})")
  endif()
endfunction()

#[[
    helios_target_apply_linker(<target>)

    Applies the detected fast linker to a single target when Helios is not
    managing the project toolchain (embedded use). No-op when the active linker
    is default, or when HELIOS_MANAGE_TOOLCHAIN already applied it globally.
]]
function(helios_target_apply_linker TARGET)
  if(HELIOS_MANAGE_TOOLCHAIN)
    return()
  endif()
  if(NOT HELIOS_ACTIVE_LINKER OR HELIOS_ACTIVE_LINKER STREQUAL "default")
    return()
  endif()

  if(NOT TARGET ${TARGET})
    return()
  endif()
  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "UTILITY")
    return()
  endif()

  if(HELIOS_ACTIVE_LINKER STREQUAL "lld-link")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
      set_property(TARGET ${TARGET} PROPERTY LINKER_TYPE LLD)
    elseif(HELIOS_LLD_LINK_EXECUTABLE)
      # CMake < 3.29: cannot set per-target MSVC linker without project-wide CMAKE_LINKER.
      # Skip silently; parent can set HELIOS_MANAGE_TOOLCHAIN=ON if they want lld-link.
    endif()
  elseif(HELIOS_ACTIVE_LINKER STREQUAL "mold" OR HELIOS_ACTIVE_LINKER STREQUAL "lld")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.29")
      if(HELIOS_ACTIVE_LINKER STREQUAL "mold")
        set_property(TARGET ${TARGET} PROPERTY LINKER_TYPE MOLD)
      else()
        set_property(TARGET ${TARGET} PROPERTY LINKER_TYPE LLD)
      endif()
    endif()
    target_link_options(${TARGET} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang,AppleClang>>:-fuse-ld=${HELIOS_ACTIVE_LINKER}>
    )
  endif()
endfunction()

#[[
    helios_configure_lto_mode()

    Prepares RelWithDebInfo ThinLTO cache directory. When HELIOS_MANAGE_TOOLCHAIN
    is ON (typical top-level builds), also applies ThinLTO / parallel LTO flags
    directory-wide so Helios modules and vendored deps share one LTO mode.
    When embedded (manage off), only helios_target_apply_lto_mode() is used.
]]
function(helios_configure_lto_mode)
  if(NOT HELIOS_ENABLE_LTO OR NOT HELIOS_IPO_SUPPORTED)
    return()
  endif()

  if(DEFINED HELIOS_LTO_MODE_CACHED)
    return()
  endif()

  set(_lto_cache_dir "${CMAKE_BINARY_DIR}/lto-cache")
  file(MAKE_DIRECTORY "${_lto_cache_dir}")
  set(HELIOS_LTO_CACHE_DIR "${_lto_cache_dir}" CACHE INTERNAL "ThinLTO cache directory")

  if(HELIOS_MANAGE_TOOLCHAIN)
    # Directory-scope under Helios only — does not affect parent project targets.
    add_compile_options(
        $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RelWithDebInfo>>:-flto=auto>
    )
    add_link_options(
        $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RelWithDebInfo>>:-flto=auto>
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/LTCG:INCREMENTAL>
    )
    if(WIN32)
      add_link_options(
          $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:RelWithDebInfo>>:/lldltocache:${_lto_cache_dir}>
      )
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
      add_link_options(
          $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-Wl,-cache_path_lto,${_lto_cache_dir}>
      )
    else()
      add_link_options(
          $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-Wl,--thinlto-cache-dir=${_lto_cache_dir}>
      )
    endif()
  endif()

  set(HELIOS_LTO_MODE_CACHED TRUE CACHE INTERNAL "RelWithDebInfo LTO mode configured")
  message(STATUS "LTO mode: RelWithDebInfo uses ThinLTO/parallel LTO (cache: ${_lto_cache_dir}); Release uses full LTO")
endfunction()

#[[
    helios_target_apply_lto_mode(<target>)

    RelWithDebInfo: ThinLTO (Clang) / -flto=auto (GCC) / /LTCG:INCREMENTAL (MSVC).
    Release keeps CMake's full IPO flags from INTERPROCEDURAL_OPTIMIZATION.
    Appended after CMake IPO flags so the last -flto* wins.
]]
function(helios_target_apply_lto_mode TARGET)
  if(NOT HELIOS_ENABLE_LTO OR NOT HELIOS_IPO_SUPPORTED)
    return()
  endif()
  # Top-level builds apply ThinLTO directory-wide in helios_configure_lto_mode().
  if(HELIOS_MANAGE_TOOLCHAIN)
    return()
  endif()
  if(NOT TARGET ${TARGET})
    return()
  endif()
  get_target_property(_type ${TARGET} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "UTILITY")
    return()
  endif()

  if(NOT HELIOS_LTO_CACHE_DIR)
    set(HELIOS_LTO_CACHE_DIR "${CMAKE_BINARY_DIR}/lto-cache")
    file(MAKE_DIRECTORY "${HELIOS_LTO_CACHE_DIR}")
  endif()

  target_compile_options(${TARGET} PRIVATE
      $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
      $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RelWithDebInfo>>:-flto=auto>
  )
  target_link_options(${TARGET} PRIVATE
      $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-flto=thin>
      $<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RelWithDebInfo>>:-flto=auto>
      $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:RelWithDebInfo>>:/LTCG:INCREMENTAL>
  )

  if(WIN32)
    target_link_options(${TARGET} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:Clang>,$<CONFIG:RelWithDebInfo>>:/lldltocache:${HELIOS_LTO_CACHE_DIR}>
    )
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_options(${TARGET} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-Wl,-cache_path_lto,${HELIOS_LTO_CACHE_DIR}>
    )
  else()
    target_link_options(${TARGET} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:Clang,AppleClang>,$<CONFIG:RelWithDebInfo>>:-Wl,--thinlto-cache-dir=${HELIOS_LTO_CACHE_DIR}>
    )
  endif()
endfunction()
