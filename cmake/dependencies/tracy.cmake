# Tracy auto-enables ROCm GPU hooks when rocprofiler-sdk is installed; disable for
# CPU-only Helios profiling to avoid loading ROCm at startup.
set(CMAKE_DISABLE_FIND_PACKAGE_rocprofiler-sdk TRUE)

helios_dependency(
    NAME Tracy
    VERSION "^0.13.0"

    INSTALL_HINTS
        apt libtracy-dev
        pacman tracy
        brew tracy
        pkg_config tracy

    CPM_REPOSITORY wolfpld/tracy
    CPM_VERSION 0.13.1
    CPM_GIT_TAG v0.13.1
    CPM_OPTIONS
        "TRACY_ENABLE ON"
        "TRACY_ON_DEMAND ON"
        "TRACY_STATIC ON"

    ALIASES
        helios::tracy::TracyClient Tracy::TracyClient
        helios::tracy Tracy::TracyClient
)

if(TARGET helios::tracy::TracyClient AND NOT TARGET helios::tracy)
  add_library(_helios_tracy_all INTERFACE)
  target_link_libraries(_helios_tracy_all INTERFACE helios::tracy::TracyClient)
  add_library(helios::tracy ALIAS _helios_tracy_all)
endif()

if(TARGET TracyClient)
  get_target_property(_helios_tracy_defs TracyClient INTERFACE_COMPILE_DEFINITIONS)
  if(_helios_tracy_defs)
    list(FILTER _helios_tracy_defs EXCLUDE REGEX "^TRACY_ROCPROF$")
    set_target_properties(TracyClient PROPERTIES
            INTERFACE_COMPILE_DEFINITIONS "${_helios_tracy_defs}")
  endif()
  get_target_property(_helios_tracy_libs TracyClient INTERFACE_LINK_LIBRARIES)
  if(_helios_tracy_libs)
    list(FILTER _helios_tracy_libs EXCLUDE REGEX "rocprofiler")
    set_target_properties(TracyClient PROPERTIES
            INTERFACE_LINK_LIBRARIES "${_helios_tracy_libs}")
  endif()
endif()
