helios_module(
    NAME core
    VERSION 0.1.0
    DESCRIPTION "Core engine module providing asserts, UUID, and other core functionality"

    HEADERS
        include/helios/assert.hpp
        include/helios/cstring_view.hpp
        include/helios/stacktrace.hpp
        include/helios/uuid.hpp

    SOURCES
        src/assert.cpp
        src/stacktrace.cpp

    PCH src/pch.hpp

    DEPENDS compiler platform utils

    USES
        stduuid PUBLIC helios::stduuid::stduuid
        Boost PUBLIC helios::boost::stacktrace

    COMPILE_DEFINITIONS
        PUBLIC
            $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:
                HELIOS_ENABLE_ASSERTS;HELIOS_ENABLE_STACKTRACE
            >

    TEST_SOURCES
        tests/assert.cpp
        tests/cstring_view.cpp
        tests/main.cpp
        tests/stacktrace.cpp
        tests/uuid.cpp
)

# Platform-specific configurations for stacktrace
if(HELIOS_USE_STL_STACKTRACE)
  message(STATUS "helios_module_core: Using C++23 STL stacktrace")
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
      target_link_libraries(helios_module_core PRIVATE ${BACKTRACE_LIBRARY})
      if(DL_LIBRARY)
        target_link_libraries(helios_module_core PRIVATE ${DL_LIBRARY})
      endif()
      if(TARGET helios::boost::stacktrace_backtrace)
        target_link_libraries(helios_module_core PRIVATE helios::boost::stacktrace_backtrace)
      endif()
      get_filename_component(BACKTRACE_INCLUDE_DIR "${BACKTRACE_LIBRARY}" DIRECTORY)
      get_filename_component(BACKTRACE_INCLUDE_DIR "${BACKTRACE_INCLUDE_DIR}/include" ABSOLUTE)
      if(EXISTS "${BACKTRACE_INCLUDE_DIR}/backtrace.h")
        target_include_directories(helios_module_core PRIVATE ${BACKTRACE_INCLUDE_DIR})
      endif()
      target_compile_definitions(helios_module_core PRIVATE BOOST_STACKTRACE_USE_BACKTRACE)
    else()
      if(TARGET helios::boost::stacktrace_addr2line)
        target_link_libraries(helios_module_core PRIVATE helios::boost::stacktrace_addr2line)
      endif()
      if(DL_LIBRARY)
        target_link_libraries(helios_module_core PRIVATE ${DL_LIBRARY})
      endif()
      target_compile_definitions(helios_module_core PRIVATE
          BOOST_STACKTRACE_USE_ADDR2LINE
      )
      message(STATUS "helios_module_core: libbacktrace not found, using Boost addr2line backend")
    endif()
  elseif(APPLE)
    target_compile_definitions(helios_module_core PRIVATE BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
  elseif(WIN32)
    target_compile_definitions(helios_module_core PRIVATE BOOST_STACKTRACE_USE_WINDBG)
  endif()
endif()

if(UNIX AND NOT APPLE)
  if(NOT HELIOS_USE_STL_STACKTRACE)
    target_link_options(helios_module_core INTERFACE
        $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:-rdynamic>
    )
  endif()
endif()
