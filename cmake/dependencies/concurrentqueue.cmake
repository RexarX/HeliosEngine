helios_dependency(
    NAME concurrentqueue
    VERSION "^1.0.0"

    INSTALL_HINTS
        apt libconcurrentqueue-dev
        brew concurrentqueue
        pkg_config concurrentqueue

    CPM_REPOSITORY cameron314/concurrentqueue
    CPM_VERSION 1.0.5
)

# Canonical include: <concurrentqueue/moodycamel/concurrentqueue.h>
#   apt / brew: /usr/include/concurrentqueue/moodycamel/concurrentqueue.h
#   CPM / upstream: SOURCE_DIR/concurrentqueue.h (flat) → build-tree shim

function(_helios_concurrentqueue_include_root_from_header _header _out_var)
  get_filename_component(_dir "${_header}" DIRECTORY)
  get_filename_component(_parent_name "${_dir}" NAME)
  if(NOT _parent_name STREQUAL "moodycamel")
    set(${_out_var} "" PARENT_SCOPE)
    return()
  endif()

  get_filename_component(_dir "${_dir}" DIRECTORY)
  get_filename_component(_parent_name "${_dir}" NAME)
  if(NOT _parent_name STREQUAL "concurrentqueue")
    set(${_out_var} "" PARENT_SCOPE)
    return()
  endif()

  get_filename_component(_include_root "${_dir}" DIRECTORY)
  set(${_out_var} "${_include_root}" PARENT_SCOPE)
endfunction()

# Resolve canonical include root from candidate directories.
# Handles both apt/brew layouts (include/concurrentqueue/moodycamel) and upstream
# CMake INSTALL_INTERFACE roots (include/concurrentqueue → moodycamel/*.h).
function(_helios_concurrentqueue_try_include_root_from_dirs _dirs _out_root _out_header)
  set(_root "")
  set(_header "")
  foreach(_dir IN LISTS _dirs)
    if(_dir MATCHES "^\\$<")
      continue()
    endif()
    if(EXISTS "${_dir}/concurrentqueue/moodycamel/concurrentqueue.h")
      set(_root "${_dir}")
      set(_header "${_dir}/concurrentqueue/moodycamel/concurrentqueue.h")
      break()
    endif()
    if(EXISTS "${_dir}/moodycamel/concurrentqueue.h")
      get_filename_component(_parent "${_dir}" DIRECTORY)
      if(EXISTS "${_parent}/concurrentqueue/moodycamel/concurrentqueue.h")
        set(_root "${_parent}")
        set(_header "${_parent}/concurrentqueue/moodycamel/concurrentqueue.h")
        break()
      endif()
    endif()
  endforeach()
  set(${_out_root} "${_root}" PARENT_SCOPE)
  set(${_out_header} "${_header}" PARENT_SCOPE)
endfunction()

if(NOT TARGET helios::lib::concurrentqueue::concurrentqueue)
  if(TARGET concurrentqueue::concurrentqueue OR TARGET concurrentqueue)
    add_library(helios::lib::concurrentqueue::concurrentqueue INTERFACE IMPORTED GLOBAL)
    set(_helios_cq_include_root "")
    set(_helios_cq_canonical_header "")

    if(DEFINED concurrentqueue_SOURCE_DIR AND concurrentqueue_SOURCE_DIR)
      if(EXISTS "${concurrentqueue_SOURCE_DIR}/concurrentqueue/moodycamel/concurrentqueue.h")
        set(_helios_cq_canonical_header
            "${concurrentqueue_SOURCE_DIR}/concurrentqueue/moodycamel/concurrentqueue.h")
      elseif(EXISTS "${concurrentqueue_SOURCE_DIR}/moodycamel/concurrentqueue.h")
        set(_helios_cq_canonical_header
            "${concurrentqueue_SOURCE_DIR}/moodycamel/concurrentqueue.h")
      elseif(EXISTS "${concurrentqueue_SOURCE_DIR}/concurrentqueue.h")
        set(_helios_cq_flat_source_dir "${concurrentqueue_SOURCE_DIR}")
      endif()
    endif()

    if(NOT _helios_cq_canonical_header AND NOT _helios_cq_flat_source_dir)
      set(_helios_cq_system_hints "")
      if(TARGET concurrentqueue::concurrentqueue)
        get_target_property(_helios_cq_target_includes
            concurrentqueue::concurrentqueue INTERFACE_INCLUDE_DIRECTORIES)
        if(_helios_cq_target_includes AND NOT _helios_cq_target_includes MATCHES "-NOTFOUND$")
          list(APPEND _helios_cq_system_hints ${_helios_cq_target_includes})
        endif()
      endif()

      # CONFIG packages (Homebrew/vcpkg) expose INSTALL_INTERFACE generator expressions in
      # INTERFACE_INCLUDE_DIRECTORIES; derive the install prefix from the package dir instead.
      if(concurrentqueue_DIR)
        get_filename_component(_helios_cq_pkg_root "${concurrentqueue_DIR}/../../.." ABSOLUTE)
        list(APPEND _helios_cq_system_hints "${_helios_cq_pkg_root}/include")
      endif()
      foreach(_helios_cq_std_include IN ITEMS
          /opt/homebrew/include
          /usr/local/include
          /usr/include)
        list(APPEND _helios_cq_system_hints "${_helios_cq_std_include}")
      endforeach()

      find_path(_helios_cq_canonical_header
          NAMES concurrentqueue.h
          HINTS ${_helios_cq_system_hints}
          PATH_SUFFIXES concurrentqueue/moodycamel moodycamel
      )

      if(NOT _helios_cq_canonical_header)
        find_path(_helios_cq_include_root
            NAMES concurrentqueue/moodycamel/concurrentqueue.h
            HINTS ${_helios_cq_system_hints}
        )
        if(_helios_cq_include_root)
          set(_helios_cq_canonical_header
              "${_helios_cq_include_root}/concurrentqueue/moodycamel/concurrentqueue.h")
        endif()
      endif()

      if(NOT _helios_cq_canonical_header AND NOT _helios_cq_include_root)
        _helios_concurrentqueue_try_include_root_from_dirs(
            "${_helios_cq_system_hints}" _helios_cq_include_root _helios_cq_canonical_header)
      endif()
    endif()

    if(_helios_cq_flat_source_dir)
      set(_helios_cq_shim_root "${CMAKE_BINARY_DIR}/helios_shims/concurrentqueue")
      set(_helios_cq_shim_dir "${_helios_cq_shim_root}/concurrentqueue/moodycamel")
      file(MAKE_DIRECTORY "${_helios_cq_shim_dir}")

      file(GLOB _helios_cq_flat_headers "${_helios_cq_flat_source_dir}/*.h")
      foreach(_helios_cq_flat_header IN LISTS _helios_cq_flat_headers)
        file(COPY "${_helios_cq_flat_header}" DESTINATION "${_helios_cq_shim_dir}")
      endforeach()

      set(_helios_cq_include_root "${_helios_cq_shim_root}")
      helios_dep_log(STATUS
          "Created helios::lib::concurrentqueue::concurrentqueue (CPM flat → moodycamel shim)")
    elseif(_helios_cq_canonical_header AND NOT _helios_cq_include_root)
      _helios_concurrentqueue_include_root_from_header(
          "${_helios_cq_canonical_header}" _helios_cq_include_root)
    endif()

    if(TARGET concurrentqueue::concurrentqueue)
      helios_dep_log(STATUS
          "Created helios::lib::concurrentqueue::concurrentqueue → concurrentqueue::concurrentqueue")
      target_link_libraries(helios::lib::concurrentqueue::concurrentqueue
          INTERFACE concurrentqueue::concurrentqueue)
    elseif(TARGET concurrentqueue)
      helios_dep_log(STATUS "Created helios::lib::concurrentqueue::concurrentqueue → concurrentqueue")
      target_link_libraries(helios::lib::concurrentqueue::concurrentqueue INTERFACE concurrentqueue)
    endif()

    if(_helios_cq_include_root)
      target_include_directories(helios::lib::concurrentqueue::concurrentqueue SYSTEM INTERFACE
          "${_helios_cq_include_root}")
      helios_dep_log(STATUS
          "concurrentqueue: <concurrentqueue/moodycamel/concurrentqueue.h> via ${_helios_cq_include_root}")
    else()
      message(WARNING
          "concurrentqueue: could not locate moodycamel/concurrentqueue.h to normalize include path")
    endif()
  endif()
endif()

if(TARGET helios::lib::concurrentqueue::concurrentqueue AND NOT TARGET helios::lib::concurrentqueue)
  add_library(_helios_concurrentqueue_all INTERFACE)
  target_link_libraries(_helios_concurrentqueue_all INTERFACE helios::lib::concurrentqueue::concurrentqueue)
  add_library(helios::lib::concurrentqueue ALIAS _helios_concurrentqueue_all)
endif()
