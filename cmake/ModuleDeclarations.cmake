# Declares modules emitted by scripts/helios_modules.py.

include_guard(GLOBAL)

function(helios_declare_module)
  cmake_parse_arguments(ARG
      ""
      "NAME;PATH;TARGET;ALIAS;VERSION;DESCRIPTION;ORIGIN;C_API;DEFAULT"
      "LANGUAGES;DEPENDS_PUBLIC;DEPENDS_PRIVATE;OPTIONAL_PUBLIC;OPTIONAL_PRIVATE;IMPLEMENTS;BACKEND_WINS"
      ${ARGN})

  if(NOT ARG_NAME)
    message(FATAL_ERROR "helios_declare_module: NAME is required")
  endif()

  string(TOUPPER "${ARG_NAME}" _upper)
  if(NOT DEFINED HELIOS_BUILD_${_upper})
    if(ARG_DEFAULT STREQUAL "OFF")
      set(_default OFF)
    else()
      set(_default ON)
    endif()
    option(HELIOS_BUILD_${_upper} "Build the ${ARG_NAME} module" ${_default})
  endif()
  if(DEFINED HELIOS_BUILD_TESTS)
    set(_tests_default ${HELIOS_BUILD_TESTS})
  else()
    set(_tests_default OFF)
  endif()
  option(${_upper}_BUILD_TESTS "Build tests for ${ARG_NAME}" ${_tests_default})

  set(${_upper}_TARGET "${ARG_TARGET}" CACHE INTERNAL "")
  set(${_upper}_ALIAS "${ARG_ALIAS}" CACHE INTERNAL "")
  set(${_upper}_PATH "${ARG_PATH}" CACHE INTERNAL "")
  set(${_upper}_VERSION "${ARG_VERSION}" CACHE INTERNAL "")
  set(${_upper}_DESCRIPTION "${ARG_DESCRIPTION}" CACHE INTERNAL "")
  set(${_upper}_ORIGIN "${ARG_ORIGIN}" CACHE INTERNAL "")
  set(${_upper}_C_API "${ARG_C_API}" CACHE INTERNAL "")
  set(${_upper}_LANGUAGES "${ARG_LANGUAGES}" CACHE INTERNAL "")
  set(${_upper}_DEPENDS_PUBLIC "${ARG_DEPENDS_PUBLIC}" CACHE INTERNAL "")
  set(${_upper}_DEPENDS_PRIVATE "${ARG_DEPENDS_PRIVATE}" CACHE INTERNAL "")
  set(${_upper}_OPTIONAL_PUBLIC "${ARG_OPTIONAL_PUBLIC}" CACHE INTERNAL "")
  set(${_upper}_OPTIONAL_PRIVATE "${ARG_OPTIONAL_PRIVATE}" CACHE INTERNAL "")
  set(${_upper}_IMPLEMENTS "${ARG_IMPLEMENTS}" CACHE INTERNAL "")
  set(${_upper}_BACKEND_WINS "${ARG_BACKEND_WINS}" CACHE INTERNAL "")
  set(${_upper}_DEPENDS "${ARG_DEPENDS_PUBLIC};${ARG_DEPENDS_PRIVATE}" CACHE INTERNAL "")
  set(${_upper}_REGISTERED TRUE CACHE INTERNAL "")
  string(TOUPPER "${ARG_TARGET}" _target_upper)
  set(${_upper}_AVAILABLE_DEF "${_target_upper}_AVAILABLE" CACHE INTERNAL "")

  set_property(GLOBAL PROPERTY HELIOS_FEATURES_${_upper} "")
  get_property(_registered GLOBAL PROPERTY HELIOS_REGISTERED_MODULES)
  if(NOT ARG_NAME IN_LIST _registered)
    set_property(GLOBAL APPEND PROPERTY HELIOS_REGISTERED_MODULES "${ARG_NAME}")
  endif()
endfunction()

function(helios_declare_feature)
  cmake_parse_arguments(ARG "" "MODULE;NAME;OPTION;DEFAULT;ABI;DESCRIPTION" "" ${ARGN})
  if(NOT DEFINED ${ARG_OPTION})
    option(${ARG_OPTION} "${ARG_DESCRIPTION}" ${ARG_DEFAULT})
  endif()
  string(TOUPPER "${ARG_MODULE}" _upper)
  set_property(GLOBAL APPEND PROPERTY HELIOS_FEATURES_${_upper}
      "${ARG_OPTION}|${ARG_ABI}")
endfunction()
