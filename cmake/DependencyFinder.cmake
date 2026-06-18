# Helios Engine Dependency Finder
#
# External dependency management with system→CPM fallback chain.
#
# Key APIs:
#   helios_dependency()         — declarative single-call dependency
#   helios_require_dependency() — include a dependency file (internal + escape hatch)
#   helios_dep_begin/end        — imperative pair for advanced dependency files
#
# New features:
#   - HELIOS_DEPENDENCY_PATHS / HELIOS_DEPENDENCY_OVERRIDE_PATHS
#   - Semantic version constraints (^1.2.0, ~1.2.3, >=A <B, 1.2.*)
#   - INSTALL_HINTS unified package manager hints

include_guard(GLOBAL)

cmake_policy(SET CMP0054 NEW)

# ============================================================================
# Global State
# ============================================================================

if(NOT DEFINED _HELIOS_DEPENDENCIES_FOUND)
  set(_HELIOS_DEPENDENCIES_FOUND "" CACHE INTERNAL "List of found dependencies")
endif()

option(HELIOS_DOWNLOAD_PACKAGES "Download missing packages using CPM" ON)
option(HELIOS_FORCE_DOWNLOAD_PACKAGES "Force download all packages even if system version exists" OFF)
option(HELIOS_CHECK_PACKAGE_VERSIONS "Check and enforce package version requirements" ON)

# Custom dependency search paths (user-extensible)
if(NOT DEFINED HELIOS_DEPENDENCY_PATHS)
  set(HELIOS_DEPENDENCY_PATHS "" CACHE STRING "Additional paths to search for dependency .cmake files")
endif()
if(NOT DEFINED HELIOS_DEPENDENCY_OVERRIDE_PATHS)
  set(HELIOS_DEPENDENCY_OVERRIDE_PATHS "" CACHE STRING "Override paths (checked before engine built-ins)")
endif()

# ============================================================================
# Logging Helpers
# ============================================================================

function(helios_dep_log)
  set(options "")
  set(oneValueArgs MESSAGE STATUS WARNING ERROR SUCCESS DOWNLOAD CONFIGURE FOUND NOT_FOUND)
  set(multiValueArgs "")
  cmake_parse_arguments(LOG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(LOG_MESSAGE)
    message(STATUS "${LOG_MESSAGE}")
  elseif(LOG_STATUS)
    message(STATUS "  ${LOG_STATUS}")
  elseif(LOG_WARNING)
    message(WARNING "  ${LOG_WARNING}")
  elseif(LOG_ERROR)
    message(FATAL_ERROR "  ${LOG_ERROR}")
  elseif(LOG_SUCCESS)
    message(STATUS "  ✓ ${LOG_SUCCESS}")
  elseif(LOG_DOWNLOAD)
    message(STATUS "  ⬇ ${LOG_DOWNLOAD}")
  elseif(LOG_CONFIGURE)
    message(STATUS "  → ${LOG_CONFIGURE}")
  elseif(LOG_FOUND)
    message(STATUS "  ✓ ${LOG_FOUND} found")
  elseif(LOG_NOT_FOUND)
    message(STATUS "  ✗ ${LOG_NOT_FOUND} not found")
  endif()
endfunction()

function(helios_dep_header)
  cmake_parse_arguments(HDR "" "NAME" "" ${ARGN})
  if(HDR_NAME)
    message(STATUS "Configuring ${HDR_NAME} dependency...")
  endif()
endfunction()

# ============================================================================
# Processing State (per-run, not cached)
# ============================================================================

function(helios_dep_is_processed)
  cmake_parse_arguments(ARG "" "NAME;OUTPUT_VAR" "" ${ARGN})
  if(NOT ARG_NAME OR NOT ARG_OUTPUT_VAR)
    message(FATAL_ERROR "helios_dep_is_processed requires NAME and OUTPUT_VAR")
  endif()
  string(TOUPPER "${ARG_NAME}" _upper_name)
  string(REPLACE "-" "_" _upper_name "${_upper_name}")
  get_property(_is_processed GLOBAL PROPERTY HELIOS_DEP_${_upper_name}_PROCESSED)
  if(_is_processed)
    set(${ARG_OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${ARG_OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(helios_dep_mark_processed)
  cmake_parse_arguments(ARG "" "NAME" "" ${ARGN})
  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_dep_mark_processed: NAME is required")
  endif()
  string(TOUPPER "${ARG_NAME}" _upper_name)
  string(REPLACE "-" "_" _upper_name "${_upper_name}")
  set_property(GLOBAL PROPERTY HELIOS_DEP_${_upper_name}_PROCESSED TRUE)
endfunction()

function(helios_dep_is_found)
  cmake_parse_arguments(ARG "" "NAME;OUTPUT_VAR" "" ${ARGN})
  if(NOT ARG_NAME OR NOT ARG_OUTPUT_VAR)
    message(FATAL_ERROR "helios_dep_is_found requires NAME and OUTPUT_VAR")
  endif()
  string(TOUPPER "${ARG_NAME}" _upper_name)
  string(REPLACE "-" "_" _upper_name "${_upper_name}")
  if(DEFINED HELIOS_DEP_${_upper_name}_FOUND AND HELIOS_DEP_${_upper_name}_FOUND)
    set(${ARG_OUTPUT_VAR} TRUE PARENT_SCOPE)
  else()
    set(${ARG_OUTPUT_VAR} FALSE PARENT_SCOPE)
  endif()
endfunction()

function(helios_dep_mark_found)
  cmake_parse_arguments(ARG "" "NAME;VIA" "" ${ARGN})
  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_dep_mark_found: NAME is required")
  endif()
  string(TOUPPER "${ARG_NAME}" _upper_name)
  string(REPLACE "-" "_" _upper_name "${_upper_name}")
  set(HELIOS_DEP_${_upper_name}_FOUND TRUE CACHE INTERNAL "Dependency ${ARG_NAME} was found")
  if(ARG_VIA)
    set(HELIOS_DEP_${_upper_name}_FOUND_VIA "${ARG_VIA}" CACHE INTERNAL "How ${ARG_NAME} was found")
  endif()
  list(APPEND _HELIOS_DEPENDENCIES_FOUND "${ARG_NAME}")
  list(REMOVE_DUPLICATES _HELIOS_DEPENDENCIES_FOUND)
  set(_HELIOS_DEPENDENCIES_FOUND "${_HELIOS_DEPENDENCIES_FOUND}" CACHE INTERNAL "List of found dependencies")
endfunction()

# ============================================================================
# Dependency File Resolution (with custom paths)
# ============================================================================

function(_helios_resolve_dep_file DEP_NAME OUT_PATH)
  # Search order:
  # 1. HELIOS_DEPENDENCY_OVERRIDE_PATHS (consumer overrides, checked first)
  # 2. ${HELIOS_ROOT_DIR}/cmake/dependencies/ (engine built-ins)
  # 3. HELIOS_DEPENDENCY_PATHS (consumer extensions)
  # 4. Case-folded retry of each path above

  set(_candidates "")

  # Override paths
  foreach(_dir ${HELIOS_DEPENDENCY_OVERRIDE_PATHS})
    list(APPEND _candidates "${_dir}/${DEP_NAME}.cmake")
  endforeach()

  # Engine built-ins
  list(APPEND _candidates "${HELIOS_ROOT_DIR}/cmake/dependencies/${DEP_NAME}.cmake")

  # Extension paths
  foreach(_dir ${HELIOS_DEPENDENCY_PATHS})
    list(APPEND _candidates "${_dir}/${DEP_NAME}.cmake")
  endforeach()

  # Case-folded retry (lowercase)
  string(TOLOWER "${DEP_NAME}" _dep_lower)
  foreach(_dir ${HELIOS_DEPENDENCY_OVERRIDE_PATHS})
    list(APPEND _candidates "${_dir}/${_dep_lower}.cmake")
  endforeach()
  list(APPEND _candidates "${HELIOS_ROOT_DIR}/cmake/dependencies/${_dep_lower}.cmake")
  foreach(_dir ${HELIOS_DEPENDENCY_PATHS})
    list(APPEND _candidates "${_dir}/${_dep_lower}.cmake")
  endforeach()

  foreach(_cand ${_candidates})
    if(EXISTS "${_cand}")
      set(${OUT_PATH} "${_cand}" PARENT_SCOPE)
      return()
    endif()
  endforeach()

  set(${OUT_PATH} "" PARENT_SCOPE)
endfunction()

macro(helios_require_dependency _dep_name)
  helios_dep_is_processed(NAME "${_dep_name}" OUTPUT_VAR _already_processed)

  if(NOT _already_processed)
    _helios_resolve_dep_file("${_dep_name}" _dep_file)
    if(_dep_file)
      include("${_dep_file}")
    else()
      message(WARNING "Dependency file not found for '${_dep_name}'")
    endif()
  endif()

  unset(_already_processed)
  unset(_dep_file)
endmacro()

# ============================================================================
# Version Constraint Parser
# ============================================================================

#[[
    helios_parse_version_constraint(VERSION_STRING OUT_MIN OUT_MAX OUT_MIN_INCL OUT_MAX_INCL)

    Parses Conan-style version constraints:
      ^1.2.0  → >=1.2.0 <2.0.0 (compatible release)
      ~1.2.3  → >=1.2.3 <1.3.0 (patch-flexible)
      >=A <B  → explicit range
      1.2.*   → >=1.2.0 <1.3.0 (wildcard suffix)
      1.4.0   → >=1.4.0 exact
]]
function(helios_parse_version_constraint VERSION_STR OUT_MIN OUT_MAX OUT_MIN_INCL OUT_MAX_INCL)
  set(_min "")
  set(_max "")
  set(_min_incl TRUE)
  set(_max_incl FALSE)

  string(STRIP "${VERSION_STR}" _v)

  if(_v MATCHES "^\\^([0-9]+\\.[0-9]+\\.[0-9]+)$")
    # ^1.2.0 → >=1.2.0 <2.0.0
    set(_min "${CMAKE_MATCH_1}")
    set(_min_incl TRUE)
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" _major "${_min}")
    math(EXPR _next_major "${_major} + 1")
    set(_max "${_next_major}.0.0")
    set(_max_incl FALSE)

  elseif(_v MATCHES "^~([0-9]+\\.[0-9]+\\.[0-9]+)$")
    # ~1.2.3 → >=1.2.3 <1.3.0
    set(_min "${CMAKE_MATCH_1}")
    set(_min_incl TRUE)
    string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\..*" "\\1" _major "${_min}")
    string(REGEX REPLACE "^([0-9]+)\\.([0-9]+)\\..*" "\\2" _minor "${_min}")
    math(EXPR _next_minor "${_minor} + 1")
    set(_max "${_major}.${_next_minor}.0")
    set(_max_incl FALSE)

  elseif(_v MATCHES "^>=[ \t]*([0-9]+(\\.[0-9]+)*)[ \t]*<[ \t]*([0-9]+(\\.[0-9]+)*)$")
    # >=A <B (explicit range)
    set(_min "${CMAKE_MATCH_1}")
    set(_max "${CMAKE_MATCH_3}")
    set(_min_incl TRUE)
    set(_max_incl FALSE)

  elseif(_v MATCHES "^>=([0-9]+(\\.[0-9]+)*)$")
    # >=A (lower bound only)
    set(_min "${CMAKE_MATCH_1}")
    set(_min_incl TRUE)
    set(_max "")
    set(_max_incl TRUE)

  elseif(_v MATCHES "^<=([0-9]+(\\.[0-9]+)*)$")
    # <=A (upper bound only)
    set(_max "${CMAKE_MATCH_1}")
    set(_max_incl TRUE)

  elseif(_v MATCHES "^([0-9]+\\.[0-9]+)\\.\\*$")
    # 1.2.* → >=1.2.0 <1.3.0
    set(_major_minor "${CMAKE_MATCH_1}")
    set(_min "${_major_minor}.0")
    set(_min_incl TRUE)
    string(REGEX REPLACE "^([0-9]+)\\..*" "\\1" _major "${_major_minor}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+)$" "\\1" _minor "${_major_minor}")
    math(EXPR _next_minor "${_minor} + 1")
    set(_max "${_major}.${_next_minor}.0")
    set(_max_incl FALSE)

  elseif(_v MATCHES "^([0-9]+\\.[0-9]+\\.[0-9]+)$")
    # Exact: 1.4.0 → >=1.4.0
    set(_min "${CMAKE_MATCH_1}")
    set(_min_incl TRUE)
    set(_max "")
    set(_max_incl TRUE)

  else()
    message(WARNING "Unrecognised version constraint: '${VERSION_STR}' — treating as exact")
    set(_min "${VERSION_STR}")
    set(_min_incl TRUE)
  endif()

  set(${OUT_MIN} "${_min}" PARENT_SCOPE)
  set(${OUT_MAX} "${_max}" PARENT_SCOPE)
  set(${OUT_MIN_INCL} "${_min_incl}" PARENT_SCOPE)
  set(${OUT_MAX_INCL} "${_max_incl}" PARENT_SCOPE)
endfunction()

# Helper: check if an exact version satisfies a constraint
function(_helios_version_satisfies VERSION MIN MAX MIN_INCL MAX_INCL OUT)
  if(MIN AND VERSION VERSION_LESS MIN)
    set(${OUT} FALSE PARENT_SCOPE)
    return()
  endif()
  if(MIN AND VERSION VERSION_EQUAL MIN AND NOT MIN_INCL)
    set(${OUT} FALSE PARENT_SCOPE)
    return()
  endif()
  if(MAX AND VERSION VERSION_GREATER MAX)
    set(${OUT} FALSE PARENT_SCOPE)
    return()
  endif()
  if(MAX AND VERSION VERSION_EQUAL MAX AND NOT MAX_INCL)
    set(${OUT} FALSE PARENT_SCOPE)
    return()
  endif()
  set(${OUT} TRUE PARENT_SCOPE)
endfunction()

# ============================================================================
# helios_dependency — Declarative Dependency API
# ============================================================================

#[[
    helios_dependency(
        NAME    spdlog
        VERSION "^1.12.0"

        INSTALL_HINTS
            apt    libspdlog-dev   dnf    spdlog-devel
            pacman spdlog          brew   spdlog
            vcpkg  spdlog

        CPM_REPOSITORY  gabime/spdlog
        CPM_VERSION     1.16.0
        CPM_OPTIONS
            "SPDLOG_BUILD_SHARED OFF"
            "SPDLOG_BUILD_TESTS  OFF"

        # Per-dependency CMAKE_BUILD_TYPE override (Debug, RelWithDebInfo, Release).
        # Falls back to HELIOS_DEPENDENCY_BUILD_TYPE when not set.
        # BUILD_TYPE RelWithDebInfo

        # Ordered fallback — first existing target wins for the alias
        ALIASES
            helios::spdlog  spdlog::spdlog
            helios::spdlog  spdlog::spdlog_header_only
            helios::spdlog  spdlog
    )
]]
function(helios_dependency)
  set(options "")
  set(oneValueArgs NAME VERSION CPM_REPOSITORY CPM_GITHUB_REPOSITORY CPM_VERSION CPM_URL CPM_GIT_TAG CPM_SOURCE_SUBDIR CPM_DOWNLOAD_ONLY BUILD_TYPE)
  set(multiValueArgs INSTALL_HINTS CPM_OPTIONS ALIASES)

  cmake_parse_arguments(DEP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT DEP_NAME)
    message(FATAL_ERROR "helios_dependency: NAME is required")
  endif()

  # Skip if already processed this run
  helios_dep_is_processed(NAME "${DEP_NAME}" OUTPUT_VAR _already)
  if(_already)
    # If previously found, still try to create aliases (they're per-run)
    _helios_dep_create_aliases("${DEP_NAME}" "${DEP_ALIASES}")
    return()
  endif()

  helios_dep_header(NAME "${DEP_NAME}")

  # Parse version constraint if provided
  set(_ver_min "")
  set(_ver_max "")
  set(_ver_min_incl TRUE)
  set(_ver_max_incl FALSE)
  if(DEP_VERSION)
    helios_parse_version_constraint("${DEP_VERSION}" _ver_min _ver_max _ver_min_incl _ver_max_incl)
  endif()

  # Parse INSTALL_HINTS into the legacy formats for compatibility with helios_dep_begin/end
  set(_debian_names "")
  set(_rpm_names "")
  set(_pacman_names "")
  set(_brew_names "")
  set(_pkg_config_names "")
  set(_hint_state "expect_pm")
  set(_current_pm "")
  foreach(_hint ${DEP_INSTALL_HINTS})
    if(_hint STREQUAL "apt")
      set(_hint_state "apt")
    elseif(_hint STREQUAL "dnf")
      set(_hint_state "dnf")
    elseif(_hint STREQUAL "pacman")
      set(_hint_state "pacman")
    elseif(_hint STREQUAL "brew")
      set(_hint_state "brew")
    elseif(_hint STREQUAL "vcpkg")
      set(_hint_state "vcpkg")
    elseif(_hint STREQUAL "pkg_config")
      set(_hint_state "pkg_config")
    else()
      if(_hint_state STREQUAL "apt")
        list(APPEND _debian_names "${_hint}")
      elseif(_hint_state STREQUAL "dnf")
        list(APPEND _rpm_names "${_hint}")
      elseif(_hint_state STREQUAL "pacman")
        list(APPEND _pacman_names "${_hint}")
      elseif(_hint_state STREQUAL "brew")
        list(APPEND _brew_names "${_hint}")
      elseif(_hint_state STREQUAL "vcpkg" OR _hint_state STREQUAL "pkg_config")
        list(APPEND _pkg_config_names "${_hint}")
      endif()
    endif()
  endforeach()

  # Resolve CPM repository
  set(_cpm_repo "")
  if(DEP_CPM_REPOSITORY)
    set(_cpm_repo "${DEP_CPM_REPOSITORY}")
  elseif(DEP_CPM_GITHUB_REPOSITORY)
    set(_cpm_repo "${DEP_CPM_GITHUB_REPOSITORY}")
  endif()

  # Build the helios_dep_begin call
  set(_begin_args NAME ${DEP_NAME})
  if(_ver_min)
    list(APPEND _begin_args VERSION ${_ver_min})
  endif()
  if(_debian_names)
    list(APPEND _begin_args DEBIAN_NAMES ${_debian_names})
  endif()
  if(_rpm_names)
    list(APPEND _begin_args RPM_NAMES ${_rpm_names})
  endif()
  if(_pacman_names)
    list(APPEND _begin_args PACMAN_NAMES ${_pacman_names})
  endif()
  if(_brew_names)
    list(APPEND _begin_args BREW_NAMES ${_brew_names})
  endif()
  if(_pkg_config_names)
    list(APPEND _begin_args PKG_CONFIG_NAMES ${_pkg_config_names})
  endif()

  if(DEP_CPM_VERSION OR _cpm_repo)
    if(DEP_CPM_VERSION)
      list(APPEND _begin_args CPM_NAME ${DEP_NAME})
      list(APPEND _begin_args CPM_VERSION ${DEP_CPM_VERSION})
    endif()
    if(_cpm_repo)
      list(APPEND _begin_args CPM_GITHUB_REPOSITORY ${_cpm_repo})
    endif()
    if(DEP_CPM_GIT_TAG)
      list(APPEND _begin_args CPM_GIT_TAG ${DEP_CPM_GIT_TAG})
    endif()
    if(DEP_CPM_URL)
      list(APPEND _begin_args CPM_URL ${DEP_CPM_URL})
    endif()
    if(DEP_CPM_SOURCE_SUBDIR)
      list(APPEND _begin_args CPM_SOURCE_SUBDIR ${DEP_CPM_SOURCE_SUBDIR})
    endif()
    if(DEP_CPM_OPTIONS)
      list(APPEND _begin_args CPM_OPTIONS ${DEP_CPM_OPTIONS})
    endif()
    if(DEP_CPM_DOWNLOAD_ONLY)
      list(APPEND _begin_args CPM_DOWNLOAD_ONLY)
    endif()
  endif()

  # Per-dependency build type; falls back to the global default
  include(DownloadUsingCPM)
  _helios_resolve_dependency_build_type("${DEP_BUILD_TYPE}" _helios_dep_build_type)

  helios_dep_begin(${_begin_args})
  helios_dep_end()

  # Check version satisfies the constraint
  if(DEP_VERSION AND ${DEP_NAME}_FOUND AND ${DEP_NAME}_VERSION)
    _helios_version_satisfies("${${DEP_NAME}_VERSION}" "${_ver_min}" "${_ver_max}" "${_ver_min_incl}" "${_ver_max_incl}" _satisfies)
    if(NOT _satisfies)
      message(WARNING "${DEP_NAME} version ${${DEP_NAME}_VERSION} does not satisfy constraint ${DEP_VERSION}")
    endif()
  endif()

  # Create aliases
  if(${DEP_NAME}_FOUND)
    _helios_dep_create_aliases("${DEP_NAME}" "${DEP_ALIASES}")
  endif()
endfunction()

# Internal: Create alias targets from ALIASES list
function(_helios_dep_create_aliases DEP_NAME ALIASES_LIST)
  if(NOT ALIASES_LIST)
    return()
  endif()

  set(_cur_helios "")
  set(_cur_sources "")

  foreach(_item ${ALIASES_LIST})
    if(_item MATCHES "^helios::")
      if(_cur_helios AND _cur_sources)
        _helios_dep_create_single_alias("${_cur_helios}" "${_cur_sources}")
      endif()
      set(_cur_helios "${_item}")
      set(_cur_sources "")
    else()
      list(APPEND _cur_sources "${_item}")
    endif()
  endforeach()

  if(_cur_helios AND _cur_sources)
    _helios_dep_create_single_alias("${_cur_helios}" "${_cur_sources}")
  endif()
endfunction()

# Internal: Create a single helios:: alias from a list of possible source targets
function(_helios_dep_create_single_alias HELIOS_TARGET SOURCE_TARGETS)
  if(TARGET ${HELIOS_TARGET})
    return()
  endif()

  foreach(_src ${SOURCE_TARGETS})
    if(TARGET ${_src})
      get_target_property(_aliased ${_src} ALIASED_TARGET)
      if(_aliased AND NOT _aliased MATCHES "-NOTFOUND$")
        add_library(${HELIOS_TARGET} ALIAS ${_aliased})
        _helios_mark_target_system("${_aliased}")
      else()
        add_library(${HELIOS_TARGET} ALIAS ${_src})
        _helios_mark_target_system("${_src}")
      endif()
      helios_dep_log(STATUS "Created alias: ${HELIOS_TARGET} → ${_src}")
      return()
    endif()
  endforeach()

  helios_dep_log(NOT_FOUND "${HELIOS_TARGET} (no source target found)")
endfunction()

# Internal: mark a target's INTERFACE_INCLUDE_DIRECTORIES as SYSTEM
function(_helios_mark_target_system TARGET)
  if(NOT TARGET ${TARGET})
    return()
  endif()

  set(_actual ${TARGET})
  get_target_property(_aliased ${TARGET} ALIASED_TARGET)
  if(_aliased AND NOT _aliased MATCHES "-NOTFOUND$")
    set(_actual ${_aliased})
  endif()

  get_target_property(_type ${_actual} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY" OR _type STREQUAL "STATIC_LIBRARY"
       OR _type STREQUAL "SHARED_LIBRARY" OR _type STREQUAL "OBJECT_LIBRARY"
       OR _type STREQUAL "UNKNOWN_LIBRARY")
    get_target_property(_includes ${_actual} INTERFACE_INCLUDE_DIRECTORIES)
    if(_includes AND NOT _includes MATCHES "-NOTFOUND$")
      set_target_properties(${_actual} PROPERTIES
                INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_includes}"
            )
    endif()
  endif()
endfunction()

# ============================================================================
# helios_dep_begin / helios_dep_end (for advanced dependency files)
# ============================================================================

macro(helios_dep_begin)
  set(options CPM_DOWNLOAD_ONLY)
  set(oneValueArgs NAME VERSION CPM_NAME CPM_VERSION CPM_GITHUB_REPOSITORY CPM_URL CPM_GIT_TAG CPM_SOURCE_SUBDIR)
  set(multiValueArgs DEBIAN_NAMES RPM_NAMES PACMAN_NAMES BREW_NAMES PKG_CONFIG_NAMES CPM_OPTIONS)

  cmake_parse_arguments(HELIOS_PKG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(_PKG_NAME "${HELIOS_PKG_NAME}")

  helios_dep_is_processed(NAME "${_PKG_NAME}" OUTPUT_VAR _PKG_ALREADY_PROCESSED)
  if(_PKG_ALREADY_PROCESSED)
    helios_dep_is_found(NAME "${_PKG_NAME}" OUTPUT_VAR _pkg_was_found)
    if(_pkg_was_found)
      set("${_PKG_NAME}_FOUND" TRUE)
    endif()
    set("${_PKG_NAME}_SKIP_HELIOS_FIND" TRUE)
    unset(_pkg_was_found)
  else()
    if(NOT HELIOS_PKG_CPM_NAME)
      set(HELIOS_PKG_CPM_NAME "${HELIOS_PKG_NAME}")
    endif()

    string(TOUPPER "${HELIOS_PKG_CPM_NAME}" _PKG_CPM_NAME_UPPER)
    string(REPLACE "-" "_" _PKG_CPM_NAME_UPPER "${_PKG_CPM_NAME_UPPER}")

    string(TOUPPER "${_PKG_NAME}" _PKG_NAME_UPPER)
    string(REPLACE "-" "_" _PKG_NAME_UPPER "${_PKG_NAME_UPPER}")
    set(_PKG_FORCE_CPM FALSE)
    if(DEFINED HELIOS_DEP_${_PKG_NAME_UPPER}_FOUND_VIA AND
           HELIOS_DEP_${_PKG_NAME_UPPER}_FOUND_VIA STREQUAL "CPM")
      set(_PKG_FORCE_CPM TRUE)
    endif()

    option(
            HELIOS_DOWNLOAD_${_PKG_CPM_NAME_UPPER}
            "Download and setup ${HELIOS_PKG_CPM_NAME} if not found"
            ${HELIOS_DOWNLOAD_PACKAGES}
        )
    option(
            HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER}
            "Force download ${HELIOS_PKG_CPM_NAME} even if system package exists"
            ${HELIOS_FORCE_DOWNLOAD_PACKAGES}
        )

    if(HELIOS_PKG_VERSION)
      if(NOT ${_PKG_NAME}_FIND_VERSION OR "${${_PKG_NAME}_FIND_VERSION}" VERSION_LESS "${HELIOS_PKG_VERSION}")
        set("${_PKG_NAME}_FIND_VERSION" "${HELIOS_PKG_VERSION}")
      endif()
    endif()

    if(NOT HELIOS_CHECK_PACKAGE_VERSIONS)
      unset("${_PKG_NAME}_FIND_VERSION")
    endif()

    if(TARGET ${_PKG_NAME} OR TARGET helios::${_PKG_NAME})
      if(NOT ${_PKG_NAME}_FIND_VERSION)
        set("${_PKG_NAME}_FOUND" ON)
        set("${_PKG_NAME}_SKIP_HELIOS_FIND" ON)
        helios_dep_mark_processed(NAME "${_PKG_NAME}")
        helios_dep_mark_found(NAME "${_PKG_NAME}" VIA "existing target")
        return()
      endif()
    endif()

    set(_ERROR_MESSAGE "Could not find `${_PKG_NAME}` package.")
    if(HELIOS_PKG_DEBIAN_NAMES)
      list(JOIN HELIOS_PKG_DEBIAN_NAMES " " _pkg_names)
      string(APPEND _ERROR_MESSAGE "\n\tDebian/Ubuntu: sudo apt install ${_pkg_names}")
    endif()
    if(HELIOS_PKG_RPM_NAMES)
      list(JOIN HELIOS_PKG_RPM_NAMES " " _pkg_names)
      string(APPEND _ERROR_MESSAGE "\n\tFedora/RHEL: sudo dnf install ${_pkg_names}")
    endif()
    if(HELIOS_PKG_PACMAN_NAMES)
      list(JOIN HELIOS_PKG_PACMAN_NAMES " " _pkg_names)
      string(APPEND _ERROR_MESSAGE "\n\tArch Linux: sudo pacman -S ${_pkg_names}")
    endif()
    if(HELIOS_PKG_BREW_NAMES)
      list(JOIN HELIOS_PKG_BREW_NAMES " " _pkg_names)
      string(APPEND _ERROR_MESSAGE "\n\tmacOS: brew install ${_pkg_names}")
    endif()
    string(APPEND _ERROR_MESSAGE "\n")

    set("${_PKG_NAME}_LIBRARIES")
    set("${_PKG_NAME}_INCLUDE_DIRS")
    set("${_PKG_NAME}_EXECUTABLE")
    set("${_PKG_NAME}_FOUND" FALSE)
  endif()
endmacro()

macro(helios_dep_end)
  if(${_PKG_NAME}_SKIP_HELIOS_FIND)
    return()
  endif()

  set(_FOUND_VIA "")

  # 1. System packages
  if(NOT ${_PKG_NAME}_FOUND AND NOT HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER} AND NOT _PKG_FORCE_CPM)
    find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} CONFIG QUIET)
    if(${_PKG_NAME}_FOUND)
      set(_FOUND_VIA "system (CONFIG)")
    else()
      find_package(${_PKG_NAME} ${${_PKG_NAME}_FIND_VERSION} MODULE QUIET)
      if(${_PKG_NAME}_FOUND)
        set(_FOUND_VIA "system (MODULE)")
      endif()
    endif()
  endif()

  # 2. pkg-config
  if(NOT ${_PKG_NAME}_FOUND AND HELIOS_PKG_PKG_CONFIG_NAMES AND NOT HELIOS_FORCE_DOWNLOAD_${_PKG_CPM_NAME_UPPER} AND NOT _PKG_FORCE_CPM)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
      list(GET HELIOS_PKG_PKG_CONFIG_NAMES 0 _pkg_config_name)
      pkg_check_modules(${_PKG_NAME}_PC QUIET IMPORTED_TARGET ${_pkg_config_name})
      if(${_PKG_NAME}_PC_FOUND)
        set(${_PKG_NAME}_FOUND TRUE)
        set(_FOUND_VIA "pkg-config")
        if(NOT TARGET ${_PKG_NAME})
          add_library(${_PKG_NAME} INTERFACE IMPORTED)
          target_link_libraries(${_PKG_NAME} INTERFACE PkgConfig::${_PKG_NAME}_PC)
        endif()
      endif()
    endif()
  endif()

  # 3. Manual search
  set(_required_vars)
  if(NOT "${${_PKG_NAME}_LIBRARIES}" STREQUAL "")
    list(APPEND _required_vars "${_PKG_NAME}_LIBRARIES")
  endif()
  if(NOT "${${_PKG_NAME}_INCLUDE_DIRS}" STREQUAL "")
    list(APPEND _required_vars "${_PKG_NAME}_INCLUDE_DIRS")
  endif()
  if(NOT "${${_PKG_NAME}_EXECUTABLE}" STREQUAL "")
    list(APPEND _required_vars "${_PKG_NAME}_EXECUTABLE")
  endif()

  if(_required_vars AND NOT ${_PKG_NAME}_FOUND AND NOT _PKG_FORCE_CPM)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
            ${_PKG_NAME}
            REQUIRED_VARS ${_required_vars}
            VERSION_VAR ${_PKG_NAME}_VERSION
            FAIL_MESSAGE "${_ERROR_MESSAGE}"
        )
    if(${_PKG_NAME}_FOUND)
      set(_FOUND_VIA "manual search")
    endif()
  endif()

  # 4. CPM
  if(NOT ${_PKG_NAME}_FOUND AND HELIOS_DOWNLOAD_${_PKG_CPM_NAME_UPPER})
    if(HELIOS_PKG_CPM_GITHUB_REPOSITORY OR HELIOS_PKG_CPM_URL)
      include(DownloadUsingCPM)
      # Use the public helios_cpm_add_package macro from DownloadUsingCPM
      _helios_cpm_add_package()
      if(${_PKG_NAME}_ADDED OR TARGET ${_PKG_NAME})
        set(${_PKG_NAME}_FOUND TRUE)
        set(_FOUND_VIA "CPM")
      endif()
    endif()
  endif()

  helios_dep_mark_processed(NAME "${_PKG_NAME}")

  if(${_PKG_NAME}_FOUND)
    helios_dep_mark_found(NAME "${_PKG_NAME}" VIA "${_FOUND_VIA}")
    helios_dep_log(SUCCESS "${_PKG_NAME} found via ${_FOUND_VIA}")

    # Mark found targets as SYSTEM to suppress third-party warnings
    if(TARGET ${_PKG_NAME})
      _helios_mark_target_system(${_PKG_NAME})
    endif()
    if(TARGET ${_PKG_NAME}::${_PKG_NAME})
      _helios_mark_target_system(${_PKG_NAME}::${_PKG_NAME})
    endif()

    if(_required_vars AND NOT TARGET ${_PKG_NAME})
      add_library(${_PKG_NAME} INTERFACE IMPORTED GLOBAL)
      if(${_PKG_NAME}_INCLUDE_DIRS)
        set_target_properties(${_PKG_NAME} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${${_PKG_NAME}_INCLUDE_DIRS}"
                    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${${_PKG_NAME}_INCLUDE_DIRS}"
                )
      endif()
      if(${_PKG_NAME}_LIBRARIES)
        set_target_properties(${_PKG_NAME} PROPERTIES
                    INTERFACE_LINK_LIBRARIES "${${_PKG_NAME}_LIBRARIES}"
                )
      endif()
    endif()
  else()
    if(${_PKG_NAME}_FIND_REQUIRED)
      message(FATAL_ERROR "${_ERROR_MESSAGE}")
    else()
      helios_dep_log(NOT_FOUND "${_PKG_NAME}")
    endif()
  endif()

  unset(_FOUND_VIA)
  unset(_required_vars)
  unset(_ERROR_MESSAGE)
  unset(_PKG_NAME)
  unset(_PKG_CPM_NAME_UPPER)
  unset(_PKG_NAME_UPPER)
  unset(_PKG_FORCE_CPM)
  unset(_PKG_ALREADY_PROCESSED)
endmacro()

# Internal CPM helper (delegates to DownloadUsingCPM's helios_cpm_add_package)
macro(_helios_cpm_add_package)
  set(_cpm_args NAME ${HELIOS_PKG_CPM_NAME})

  if(HELIOS_PKG_CPM_VERSION)
    list(APPEND _cpm_args VERSION ${HELIOS_PKG_CPM_VERSION})
  endif()
  if(HELIOS_PKG_CPM_GITHUB_REPOSITORY)
    list(APPEND _cpm_args GITHUB_REPOSITORY ${HELIOS_PKG_CPM_GITHUB_REPOSITORY})
  endif()
  if(HELIOS_PKG_CPM_URL)
    list(APPEND _cpm_args URL ${HELIOS_PKG_CPM_URL})
  endif()
  if(HELIOS_PKG_CPM_GIT_TAG)
    list(APPEND _cpm_args GIT_TAG ${HELIOS_PKG_CPM_GIT_TAG})
  endif()
  if(HELIOS_PKG_CPM_SOURCE_SUBDIR)
    list(APPEND _cpm_args SOURCE_SUBDIR ${HELIOS_PKG_CPM_SOURCE_SUBDIR})
  endif()
  if(HELIOS_PKG_CPM_OPTIONS)
    list(APPEND _cpm_args OPTIONS ${HELIOS_PKG_CPM_OPTIONS})
  endif()
  if(HELIOS_PKG_CPM_DOWNLOAD_ONLY)
    list(APPEND _cpm_args DOWNLOAD_ONLY YES)
  endif()
  list(APPEND _cpm_args SYSTEM)

  helios_cpm_add_package(${_cpm_args})

  unset(_cpm_args)
endmacro()

# ============================================================================
# Find helpers (for advanced dependency files)
# ============================================================================

macro(_helios_dep_find_part)
  set(options)
  set(oneValueArgs PART_TYPE)
  set(multiValueArgs NAMES PATHS PATH_SUFFIXES)
  cmake_parse_arguments(PART "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(${_PKG_NAME}_SKIP_HELIOS_FIND)
    return()
  endif()

  if("${PART_PART_TYPE}" STREQUAL "library")
    set(_variable "${_PKG_NAME}_LIBRARIES")
    find_library(${_PKG_NAME}_LIBRARIES_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
    list(APPEND "${_variable}" "${${_PKG_NAME}_LIBRARIES_${PART_NAMES}}")
  elseif("${PART_PART_TYPE}" STREQUAL "include")
    set(_variable "${_PKG_NAME}_INCLUDE_DIRS")
    find_path(${_PKG_NAME}_INCLUDE_DIRS_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
    list(APPEND "${_variable}" "${${_PKG_NAME}_INCLUDE_DIRS_${PART_NAMES}}")
  elseif("${PART_PART_TYPE}" STREQUAL "program")
    set(_variable "${_PKG_NAME}_EXECUTABLE")
    find_program(${_PKG_NAME}_EXECUTABLE_${PART_NAMES}
            NAMES ${PART_NAMES}
            PATHS ${PART_PATHS}
            PATH_SUFFIXES ${PART_PATH_SUFFIXES}
        )
    list(APPEND "${_variable}" "${${_PKG_NAME}_EXECUTABLE_${PART_NAMES}}")
  endif()
endmacro()

macro(helios_dep_find_library)
  cmake_parse_arguments(LIB "" "" "NAMES;PATHS;PATH_SUFFIXES" ${ARGN})
  _helios_dep_find_part(PART_TYPE library NAMES ${LIB_NAMES} PATHS ${LIB_PATHS} PATH_SUFFIXES ${LIB_PATH_SUFFIXES})
endmacro()

macro(helios_dep_find_include)
  cmake_parse_arguments(INC "" "" "NAMES;PATHS;PATH_SUFFIXES" ${ARGN})
  _helios_dep_find_part(PART_TYPE include NAMES ${INC_NAMES} PATHS ${INC_PATHS} PATH_SUFFIXES ${INC_PATH_SUFFIXES})
endmacro()

macro(helios_dep_find_program)
  cmake_parse_arguments(PROG "" "" "NAMES;PATHS;PATH_SUFFIXES" ${ARGN})
  _helios_dep_find_part(PART_TYPE program NAMES ${PROG_NAMES} PATHS ${PROG_PATHS} PATH_SUFFIXES ${PROG_PATH_SUFFIXES})
endmacro()

# ============================================================================
# Summary
# ============================================================================

function(helios_print_dependencies)
  message(STATUS "========== Helios Engine Dependencies ==========")
  if(_HELIOS_DEPENDENCIES_FOUND)
    set(_deps_list ${_HELIOS_DEPENDENCIES_FOUND})
    list(REMOVE_DUPLICATES _deps_list)
    list(SORT _deps_list)
    foreach(_dep ${_deps_list})
      string(TOUPPER "${_dep}" _upper_dep)
      string(REPLACE "-" "_" _upper_dep "${_upper_dep}")
      if(DEFINED HELIOS_DEP_${_upper_dep}_FOUND_VIA)
        message(STATUS "  ✓ ${_dep} (${HELIOS_DEP_${_upper_dep}_FOUND_VIA})")
      else()
        message(STATUS "  ✓ ${_dep}")
      endif()
    endforeach()
  else()
    message(STATUS "  No dependencies found")
  endif()
  message(STATUS "================================================")
endfunction()
