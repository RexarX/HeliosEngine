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

if(NOT TARGET helios::concurrentqueue::concurrentqueue)
  if(TARGET concurrentqueue::concurrentqueue OR TARGET concurrentqueue)
    add_library(helios::concurrentqueue::concurrentqueue INTERFACE IMPORTED GLOBAL)
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

      find_path(_helios_cq_canonical_header
          NAMES concurrentqueue.h
          HINTS ${_helios_cq_system_hints}
          PATH_SUFFIXES concurrentqueue/moodycamel
      )
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
          "Created helios::concurrentqueue::concurrentqueue (CPM flat → moodycamel shim)")
    elseif(_helios_cq_canonical_header)
      _helios_concurrentqueue_include_root_from_header(
          "${_helios_cq_canonical_header}" _helios_cq_include_root)

      if(TARGET concurrentqueue::concurrentqueue)
        helios_dep_log(STATUS
            "Created helios::concurrentqueue::concurrentqueue → concurrentqueue::concurrentqueue")
        target_link_libraries(helios::concurrentqueue::concurrentqueue
            INTERFACE concurrentqueue::concurrentqueue)
      elseif(TARGET concurrentqueue)
        helios_dep_log(STATUS "Created helios::concurrentqueue::concurrentqueue → concurrentqueue")
        target_link_libraries(helios::concurrentqueue::concurrentqueue INTERFACE concurrentqueue)
      endif()
    endif()

    if(_helios_cq_include_root)
      target_include_directories(helios::concurrentqueue::concurrentqueue SYSTEM INTERFACE
          "${_helios_cq_include_root}")
      helios_dep_log(STATUS
          "concurrentqueue: <concurrentqueue/moodycamel/concurrentqueue.h> via ${_helios_cq_include_root}")
    else()
      message(WARNING
          "concurrentqueue: could not locate moodycamel/concurrentqueue.h to normalize include path")
    endif()
  endif()
endif()

if(TARGET helios::concurrentqueue::concurrentqueue AND NOT TARGET helios::concurrentqueue)
  add_library(_helios_concurrentqueue_all INTERFACE)
  target_link_libraries(_helios_concurrentqueue_all INTERFACE helios::concurrentqueue::concurrentqueue)
  add_library(helios::concurrentqueue ALIAS _helios_concurrentqueue_all)
endif()
