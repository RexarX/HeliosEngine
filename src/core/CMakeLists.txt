add_library(helios_core)

target_sources(helios_core PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/helios/core/core.hpp
    include/helios/assert.hpp
    include/helios/cstring_view.hpp
    include/helios/delegate.hpp
    include/helios/stacktrace.hpp
    include/helios/uuid.hpp
    include/helios/version.hpp
)

target_sources(helios_core PRIVATE
    src/assert.cpp
    src/stacktrace.cpp
    src/version.cpp
)

if(HELIOS_BUILD_C_API)
  target_sources(helios_core PUBLIC
      FILE_SET c_headers TYPE HEADERS BASE_DIRS capi/include FILES
          capi/include/helios/version.h
  )
  target_sources(helios_core PRIVATE capi/src/version.cpp)
endif()

helios_require_dependency(stduuid)
target_link_libraries(helios_core PUBLIC helios::lib::stduuid::stduuid)

helios_require_dependency(Boost)
target_link_libraries(helios_core PRIVATE helios::lib::boost::stacktrace)

# In-tree only. An installed SDK bakes the same macros from the config that
# was installed (see HeliosSdkChecks.cmake), so a Release game can still link
# the checked library and keep API checks.
target_compile_definitions(helios_core PUBLIC
    $<BUILD_INTERFACE:$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:HELIOS_ENABLE_ASSERTS>>
    $<BUILD_INTERFACE:$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:HELIOS_ENABLE_STACKTRACE>>
)

helios_apply_conventions(helios_core)
helios_target_set_folder(helios_core "Helios/Modules")
helios_target_set_output_dirs(helios_core)
helios_target_precompile_headers(helios_core src/pch.hpp)
helios_target_cpp_module_sources(helios_core modules/helios.core.cppm)

if(HELIOS_CURRENT_MODULE_BUILD_TESTS)
  helios_add_module_tests(helios_core
      SOURCES
          tests/assert.cpp
          tests/cstring_view.cpp
          tests/delegate.cpp
          tests/main.cpp
          tests/module_import.cpp
          tests/stacktrace.cpp
          tests/uuid.cpp
          tests/version.cpp

      C_SOURCES
          capi/tests/c_smoke.c
  )
endif()

set(_helios_core_generated "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/helios/version_macros.h.in"
    "${_helios_core_generated}/helios/version_macros.h"
    @ONLY
)
target_include_directories(helios_core PUBLIC
    $<BUILD_INTERFACE:$<$<COMPILE_LANGUAGE:C,CXX>:${_helios_core_generated}>>
)
if(HELIOS_ENABLE_INSTALL)
  install(FILES "${_helios_core_generated}/helios/version_macros.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/helios"
  )
endif()

# Platform-specific configurations for stacktrace
if(HELIOS_USE_STL_STACKTRACE)
  message(STATUS "helios_core: Using C++23 STL stacktrace")
else()
  if(UNIX AND NOT APPLE)
    if(NOT DEFINED HELIOS_CORE_BACKTRACE_LIBRARY)
      execute_process(
          COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libbacktrace.a
          OUTPUT_VARIABLE _backtrace_from_compiler
          OUTPUT_STRIP_TRAILING_WHITESPACE
          ERROR_QUIET
      )

      set(BACKTRACE_LIBRARY "")
      if(_backtrace_from_compiler
        AND NOT _backtrace_from_compiler STREQUAL "libbacktrace.a"
        AND EXISTS "${_backtrace_from_compiler}")
        set(BACKTRACE_LIBRARY "${_backtrace_from_compiler}")
      else()
        find_library(BACKTRACE_LIBRARY NAMES backtrace libbacktrace)
      endif()
      set(HELIOS_CORE_BACKTRACE_LIBRARY "${BACKTRACE_LIBRARY}" CACHE INTERNAL "Backtrace library path")
    else()
      set(BACKTRACE_LIBRARY "${HELIOS_CORE_BACKTRACE_LIBRARY}")
    endif()

    find_library(DL_LIBRARY NAMES dl)
    if(BACKTRACE_LIBRARY)
      target_link_libraries(helios_core PRIVATE ${BACKTRACE_LIBRARY})
      if(DL_LIBRARY)
        target_link_libraries(helios_core PRIVATE ${DL_LIBRARY})
      endif()
      if(TARGET helios::lib::boost::stacktrace_backtrace)
        target_link_libraries(helios_core PRIVATE helios::lib::boost::stacktrace_backtrace)
      endif()
      get_filename_component(BACKTRACE_LIB_DIR "${BACKTRACE_LIBRARY}" DIRECTORY)
      get_filename_component(BACKTRACE_INCLUDE_DIR "${BACKTRACE_LIB_DIR}/include" ABSOLUTE)
      if(EXISTS "${BACKTRACE_INCLUDE_DIR}/backtrace.h")
        # GCC ships backtrace.h next to a stddef.h that breaks clang -stdlib=libc++
        # when added via -I/-isystem. Expose only backtrace.h through a wrap dir.
        set(_helios_bt_wrap_dir
            "${CMAKE_CURRENT_BINARY_DIR}/helios_backtrace_include"
        )
        file(MAKE_DIRECTORY "${_helios_bt_wrap_dir}")
        configure_file(
            "${BACKTRACE_INCLUDE_DIR}/backtrace.h"
            "${_helios_bt_wrap_dir}/backtrace.h"
            COPYONLY
        )
        target_include_directories(
            helios_core PRIVATE "${_helios_bt_wrap_dir}"
        )
      endif()
      target_compile_definitions(helios_core PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)
    else()
      if(TARGET helios::lib::boost::stacktrace_addr2line)
        target_link_libraries(helios_core PRIVATE helios::lib::boost::stacktrace_addr2line)
      endif()
      if(DL_LIBRARY)
        target_link_libraries(helios_core PRIVATE ${DL_LIBRARY})
      endif()
      target_compile_definitions(helios_core PRIVATE
          BOOST_STACKTRACE_USE_ADDR2LINE
      )
      message(STATUS "helios_core: libbacktrace not found, using Boost addr2line backend")
    endif()

    target_link_options(helios_core INTERFACE
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-rdynamic>
    )
  elseif(WIN32)
    target_compile_definitions(helios_core PRIVATE BOOST_STACKTRACE_USE_WINDBG)
  endif()
endif()
