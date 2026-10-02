#ifndef HELIOS_CORE_VERSION_H
#define HELIOS_CORE_VERSION_H

#include <helios/capi/capi.h>
#include <helios/version_macros.h>

#include <stdbool.h>
#include <stdint.h>

HELIOS_C_BEGIN_DECLS

/// @brief Semantic library / header version.
/// @details Same layout and compatibility rules as `helios::Version`.
typedef struct HeliosVersion {
  uint8_t major;
  uint8_t minor;
  uint16_t patch;
} HeliosVersion;

/// @brief Gets the version of the headers visible to this translation unit.
/// @return `HeliosVersion` with `major`, `minor`, and `patch`
static inline HeliosVersion helios_header_version(void) {
  const HeliosVersion version = {
      .major = HELIOS_VERSION_MAJOR,
      .minor = HELIOS_VERSION_MINOR,
      .patch = HELIOS_VERSION_PATCH,
  };
  return version;
}

/// @brief Gets the version baked into the linked Helios binary.
/// @details Differs from `helios_header_version()` when an application is
/// compiled against newer or older headers than the loaded shared library.
/// @return Linked binary `major`, `minor`, and `patch`
HELIOS_C_API HeliosVersion helios_linked_version(void);

/// @brief Gets `"major.minor.patch"` for `helios_linked_version()`.
/// @return Null-terminated version string literal
HELIOS_C_API const char* helios_version_string(void);

/// @brief Whether a binary at `linked` can satisfy headers at `header`.
/// @param header Version of the headers the caller compiled against
/// @param linked Version of the Helios binary being used
/// @return `true` if majors match and `linked.minor >= header.minor`
static inline bool helios_versions_compatible(HeliosVersion header,
                                              HeliosVersion linked) {
  return linked.major == header.major && linked.minor >= header.minor;
}

/// @brief Checks compatibility of the headers with the linked binary.
/// @return `true` if the linked binary is compatible with the headers
static inline bool helios_compatible_with_headers(void) {
  return helios_versions_compatible(helios_header_version(),
                                    helios_linked_version());
}

HELIOS_C_END_DECLS

#endif
