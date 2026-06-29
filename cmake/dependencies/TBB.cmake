# TBB dependency configuration
#
# TBB is required on Linux when using GCC's libstdc++ because the parallel STL
# implementation uses TBB as its backend.
#
# Sourced from system packages only — no CPM download.

# Check if already processed (handled by helios_require_dependency, but we
# need early guard here to avoid duplicate processing of the body)
helios_dep_is_processed(NAME "TBB" OUTPUT_VAR _tbb_processed)
if(_tbb_processed)
  return()
endif()

include(CheckCXXSymbolExists)

# Determine the standard library in use
if(cxx_std_20 IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  set(_header version)
else()
  set(_header ciso646)
endif()

check_cxx_symbol_exists(__GLIBCXX__ ${_header} GLIBCXX)
check_cxx_symbol_exists(_LIBCPP_VERSION ${_header} LIBCPP)

# Only needed with libstdc++ on Linux
if(NOT (UNIX AND NOT APPLE AND GLIBCXX))
  helios_dep_log(STATUS "TBB: Not required (not using libstdc++)")
  helios_dep_mark_processed(NAME "TBB")
  return()
endif()

helios_dep_header(NAME "TBB (system package for libstdc++ parallel STL)")

find_package(TBB QUIET)

if(TBB_FOUND OR TARGET TBB::tbb)
  helios_dep_log(SUCCESS "TBB found via find_package")
  helios_dep_mark_found(NAME "TBB" VIA "system (find_package)")
else()
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(TBB_PKG QUIET tbb)
    if(TBB_PKG_FOUND)
      helios_dep_log(SUCCESS "TBB found via pkg-config")
      helios_dep_mark_found(NAME "TBB" VIA "system (pkg-config)")
      add_library(TBB::tbb INTERFACE IMPORTED)
      target_include_directories(TBB::tbb INTERFACE ${TBB_PKG_INCLUDE_DIRS})
      target_link_libraries(TBB::tbb INTERFACE ${TBB_PKG_LIBRARIES})
      target_link_directories(TBB::tbb INTERFACE ${TBB_PKG_LIBRARY_DIRS})
    endif()
  endif()
endif()

helios_dep_mark_processed(NAME "TBB")

if(NOT TARGET helios::lib::tbb::tbb)
  if(TARGET TBB::tbb)
    add_library(helios::lib::tbb::tbb ALIAS TBB::tbb)
    set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
  elseif(TARGET TBB::TBB)
    add_library(helios::lib::tbb::tbb ALIAS TBB::TBB)
    set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
  else()
    set(HELIOS_HAS_SYSTEM_TBB FALSE CACHE INTERNAL "System TBB is not available")
    helios_dep_log(WARNING "TBB not found - parallel STL algorithms will use serial backend")
    helios_dep_log(STATUS "Install TBB: sudo apt install libtbb-dev / sudo dnf install tbb-devel / sudo pacman -S onetbb")
  endif()
else()
  set(HELIOS_HAS_SYSTEM_TBB TRUE CACHE INTERNAL "System TBB is available")
endif()

if(NOT TARGET helios::lib::tbb)
  add_library(_helios_tbb_all INTERFACE)
  if(TARGET helios::lib::tbb::tbb)
    target_link_libraries(_helios_tbb_all INTERFACE helios::lib::tbb::tbb)
  endif()
  add_library(helios::lib::tbb ALIAS _helios_tbb_all)
endif()
