# Boost dependency configuration
#
# Handles Boost dependency + C++23 std::stacktrace detection.
# Complex custom logic — uses helios_dep_begin/end API.
#
# Usage: helios_require_dependency(Boost)

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

# Check if C++23 <stacktrace> is available
cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")
set(CMAKE_REQUIRED_LINK_OPTIONS "")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
  set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if(CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
    set(_stl_stacktrace_link_libs "")
  else()
    list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-lstdc++_libbacktrace")
    set(_stl_stacktrace_link_libs "stdc++_libbacktrace")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(_stl_stacktrace_link_libs "")
endif()

check_cxx_source_compiles("
#include <stacktrace>
#include <string>
int main() {
    auto st = std::stacktrace::current();
    if (st.size() > 0) {
        std::string desc = std::to_string(st[0]);
        (void)desc;
    }
    return 0;
}
" HELIOS_HAS_STL_STACKTRACE)

cmake_pop_check_state()

if(HELIOS_HAS_STL_STACKTRACE)
  helios_dep_log(SUCCESS "C++23 <stacktrace> header available, using STL stacktrace")
  set(HELIOS_USE_STL_STACKTRACE ON CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")

  if(NOT TARGET helios::stl_stacktrace)
    add_library(helios::stl_stacktrace INTERFACE IMPORTED GLOBAL)
    target_compile_definitions(helios::stl_stacktrace INTERFACE HELIOS_USE_STL_STACKTRACE)
    if(_stl_stacktrace_link_libs)
      target_link_libraries(helios::stl_stacktrace INTERFACE ${_stl_stacktrace_link_libs})
    endif()
  endif()

  if(NOT TARGET helios::boost::stacktrace)
    add_library(helios::boost::stacktrace INTERFACE IMPORTED GLOBAL)
    target_link_libraries(helios::boost::stacktrace INTERFACE helios::stl_stacktrace)
  endif()
else()
  helios_dep_log(STATUS "C++23 <stacktrace> not available, using Boost stacktrace")
  set(HELIOS_USE_STL_STACKTRACE OFF CACHE INTERNAL "Use C++23 STL stacktrace instead of Boost")
endif()

if(NOT HELIOS_USE_STL_STACKTRACE)
  set(HELIOS_BOOST_REQUIRED_COMPONENTS stacktrace)
else()
  set(HELIOS_BOOST_REQUIRED_COMPONENTS "")
endif()

# Try to find or download Boost
set(_boost_found_via "")
set(_boost_prefer_cpm FALSE)

if(DEFINED HELIOS_DEP_BOOST_FOUND_VIA AND HELIOS_DEP_BOOST_FOUND_VIA STREQUAL "CPM")
  set(_boost_prefer_cpm TRUE)
endif()

if(NOT _boost_prefer_cpm)
  find_package(Boost 1.87 QUIET CONFIG)
  if(Boost_FOUND)
    set(_boost_found_via "system (CONFIG)")
  else()
    if(HELIOS_BOOST_REQUIRED_COMPONENTS)
      find_package(Boost 1.87 QUIET COMPONENTS ${HELIOS_BOOST_REQUIRED_COMPONENTS})
    else()
      find_package(Boost 1.87 QUIET)
    endif()
    if(Boost_FOUND)
      set(_boost_found_via "system (MODULE)")
    endif()
  endif()
endif()

# Link a Helios Boost wrapper to the best available imported Boost target.
function(_helios_link_boost_interface _helios_target)
  if(TARGET Boost::boost)
    target_link_libraries(${_helios_target} INTERFACE Boost::boost)
  elseif(TARGET Boost::headers)
    target_link_libraries(${_helios_target} INTERFACE Boost::headers)
  elseif(Boost_INCLUDE_DIRS)
    target_include_directories(${_helios_target} SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
  endif()
endfunction()

if(Boost_FOUND)
  helios_dep_log(SUCCESS "Boost found via ${_boost_found_via}")
  helios_dep_mark_found(NAME "Boost" VIA "${_boost_found_via}")

  if(NOT TARGET helios::boost::boost)
    add_library(helios::boost::boost INTERFACE IMPORTED GLOBAL)
    _helios_link_boost_interface(helios::boost::boost)
  endif()

  if(NOT HELIOS_USE_STL_STACKTRACE)
    if(NOT TARGET helios::boost::stacktrace)
      add_library(helios::boost::stacktrace INTERFACE IMPORTED GLOBAL)
      if(TARGET Boost::stacktrace)
        target_link_libraries(helios::boost::stacktrace INTERFACE Boost::stacktrace)
      else()
        _helios_link_boost_interface(helios::boost::stacktrace)
      endif()
    endif()
  endif()

  if(NOT TARGET helios::boost::unordered)
    add_library(helios::boost::unordered INTERFACE IMPORTED GLOBAL)
    _helios_link_boost_interface(helios::boost::unordered)
  endif()

  if(NOT TARGET helios::boost::container)
    add_library(helios::boost::container INTERFACE IMPORTED GLOBAL)
    if(TARGET Boost::container)
      target_link_libraries(helios::boost::container INTERFACE Boost::container)
    else()
      _helios_link_boost_interface(helios::boost::container)
    endif()
  endif()

  helios_dep_mark_processed(NAME "Boost")
else()
  if(HELIOS_DOWNLOAD_PACKAGES)
    helios_dep_log(DOWNLOAD "Boost not found in system, downloading via CPM...")

    if(HELIOS_USE_STL_STACKTRACE)
      set(_boost_include_libs "container;unordered")
    else()
      set(_boost_include_libs "container;stacktrace;unordered")
    endif()

    # BOOST_INCLUDE_LIBRARIES is semicolon-separated; CPM must iterate OPTIONS with
    # foreach(... IN LISTS ...) so the value is not split (see cmake/CPM.cmake).
    include(DownloadUsingCPM)
    helios_cpm_add_package(
        NAME Boost
        VERSION 1.90.0
        URL https://github.com/boostorg/boost/releases/download/boost-1.90.0/boost-1.90.0-cmake.tar.xz
        OPTIONS
            "BOOST_ENABLE_CMAKE ON"
            "BOOST_INCLUDE_LIBRARIES ${_boost_include_libs}"
            "BUILD_SHARED_LIBS OFF"
            "CMAKE_POSITION_INDEPENDENT_CODE ON"
        SYSTEM
    )

    if(Boost_ADDED OR TARGET Boost::boost)
      if(NOT TARGET helios::boost::boost)
        add_library(helios::boost::boost INTERFACE IMPORTED GLOBAL)
        if(TARGET Boost::boost)
          target_link_libraries(helios::boost::boost INTERFACE Boost::boost)
        endif()
      endif()

      if(NOT HELIOS_USE_STL_STACKTRACE)
        if(NOT TARGET helios::boost::stacktrace)
          add_library(helios::boost::stacktrace INTERFACE IMPORTED GLOBAL)
        endif()
        if(TARGET Boost::stacktrace)
          target_link_libraries(helios::boost::stacktrace INTERFACE Boost::stacktrace)
        else()
          target_link_libraries(helios::boost::stacktrace INTERFACE helios::boost::boost)
        endif()
      endif()

      if(NOT TARGET helios::boost::unordered)
        add_library(helios::boost::unordered INTERFACE IMPORTED GLOBAL)
        target_link_libraries(helios::boost::unordered INTERFACE helios::boost::boost)
        if(Boost_SOURCE_DIR AND EXISTS "${Boost_SOURCE_DIR}/libs/unordered/include")
          target_include_directories(helios::boost::unordered SYSTEM INTERFACE
              "${Boost_SOURCE_DIR}/libs/unordered/include"
          )
        endif()
      endif()

      if(NOT TARGET helios::boost::container)
        add_library(helios::boost::container INTERFACE IMPORTED GLOBAL)
        if(TARGET Boost::container)
          target_link_libraries(helios::boost::container INTERFACE Boost::container)
        else()
          target_link_libraries(helios::boost::container INTERFACE helios::boost::boost)
        endif()
        if(Boost_SOURCE_DIR AND EXISTS "${Boost_SOURCE_DIR}/libs/container/include")
          target_include_directories(helios::boost::container SYSTEM INTERFACE
                        "${Boost_SOURCE_DIR}/libs/container/include")
        endif()
      endif()
    endif()

    helios_dep_mark_processed(NAME "Boost")
    helios_dep_mark_found(NAME "Boost" VIA "CPM")
  else()
    helios_dep_log(WARNING "Boost not found and HELIOS_DOWNLOAD_PACKAGES is OFF")
    helios_dep_mark_processed(NAME "Boost")
  endif()
endif()

if(NOT TARGET helios::boost)
  add_library(_helios_boost_all INTERFACE)
  if(TARGET helios::boost::boost)
    target_link_libraries(_helios_boost_all INTERFACE helios::boost::boost)
  endif()
  if(TARGET helios::boost::unordered)
    target_link_libraries(_helios_boost_all INTERFACE helios::boost::unordered)
  endif()
  if(TARGET helios::boost::stacktrace)
    target_link_libraries(_helios_boost_all INTERFACE helios::boost::stacktrace)
  endif()
  add_library(helios::boost ALIAS _helios_boost_all)
endif()

# Boost.Stacktrace platform requirements must be INTERFACE so they propagate to
# test targets that reuse module PCHs including <boost/stacktrace.hpp>.
if(NOT HELIOS_USE_STL_STACKTRACE AND TARGET helios::boost::stacktrace AND APPLE)
  target_compile_definitions(helios::boost::stacktrace INTERFACE
      BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
  )
endif()
