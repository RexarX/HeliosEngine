#ifndef HELIOS_CAPI_ERROR_H
#define HELIOS_CAPI_ERROR_H

#include <helios/capi/capi.h>
#include <helios/platform/platform.h>

#include <stdint.h>

/// @brief Recoverable failure of a valid call.
typedef uint8_t HeliosError;

enum {
  HELIOS_ERROR_OK = 0U,  ///< Success
  HELIOS_ERROR_UNKNOWN,  ///< Unspecified failure
};

/// @brief Converts `HeliosError` enumerator into readable string.
/// @param error Enumerator to convert
/// @return Enumerator name, or `"Unknown"`
static inline const char* helios_error_to_string(HeliosError error) {
  switch (error) {
    case HELIOS_ERROR_OK:
      return "Ok";
    case HELIOS_ERROR_UNKNOWN:
      return "Unknown";
    default:
      return "Unknown";
  }
}

HELIOS_C_BEGIN_DECLS

/// @brief Returns the last error message set on this thread.
/// @details Returned pointer is valid until the next Helios C API function call
/// on this thread that uses error messages.
/// @return A null-terminated string, empty when no error has been recorded.
HELIOS_C_API const char* helios_last_error_message(void);

/// @brief Stores an error message on this thread.
/// @param message Null-terminated message, or `NULL` to clear it
HELIOS_C_API void helios_set_last_error(const char* message);

HELIOS_C_END_DECLS

#endif
