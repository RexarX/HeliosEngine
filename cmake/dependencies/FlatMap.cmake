# FlatMap dependency configuration
#
# Checks for C++23 std::flat_map availability and falls back to Boost.Container.

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

cmake_push_check_state(RESET)
set(CMAKE_REQUIRED_FLAGS "${CMAKE_CXX_FLAGS}")

check_cxx_source_compiles("
#include <flat_map>
#include <string>
int main() {
    std::flat_map<int, std::string> map;
    map.insert({1, \"test\"});
    auto it = map.find(1);
    return it != map.end() ? 0 : 1;
}
" HELIOS_HAS_STL_FLAT_MAP)

cmake_pop_check_state()

if(HELIOS_HAS_STL_FLAT_MAP)
  helios_dep_log(SUCCESS "C++23 std::flat_map available, using STL flat_map")
  set(HELIOS_USE_STL_FLAT_MAP ON CACHE INTERNAL "Use C++23 STL flat_map instead of Boost")

  if(NOT TARGET helios::stl_flat_map)
    add_library(helios::stl_flat_map INTERFACE IMPORTED GLOBAL)
    target_compile_definitions(helios::stl_flat_map INTERFACE HELIOS_USE_STL_FLAT_MAP)
  endif()

  if(NOT TARGET helios::flat_map)
    add_library(helios::flat_map INTERFACE IMPORTED GLOBAL)
    target_link_libraries(helios::flat_map INTERFACE helios::stl_flat_map)
  endif()

  helios_dep_mark_found(NAME "FlatMap" VIA "STL (C++23)")
  helios_dep_mark_processed(NAME "FlatMap")
else()
  helios_dep_log(STATUS "C++23 std::flat_map not available, using Boost.Container flat_map")
  set(HELIOS_USE_STL_FLAT_MAP OFF CACHE INTERNAL "Use C++23 STL flat_map instead of Boost")

  helios_require_dependency(Boost)

  if(TARGET helios::boost::container OR TARGET Boost::boost OR TARGET helios::boost::boost)
    if(NOT TARGET helios::flat_map)
      add_library(helios::flat_map INTERFACE IMPORTED GLOBAL)
      if(TARGET helios::boost::container)
        target_link_libraries(helios::flat_map INTERFACE helios::boost::container)
      elseif(TARGET helios::boost::boost)
        target_link_libraries(helios::flat_map INTERFACE helios::boost::boost)
      elseif(TARGET Boost::container)
        target_link_libraries(helios::flat_map INTERFACE Boost::container)
      elseif(TARGET Boost::boost)
        target_link_libraries(helios::flat_map INTERFACE Boost::boost)
      endif()
    endif()

    helios_dep_mark_found(NAME "FlatMap" VIA "Boost.Container")
    helios_dep_mark_processed(NAME "FlatMap")
  else()
    helios_dep_log(WARNING "FlatMap: Neither std::flat_map nor Boost.Container available")
    helios_dep_mark_processed(NAME "FlatMap")
  endif()
endif()
