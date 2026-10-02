#include <helios/capi/capi.h>
#include <helios/version.h>

#ifdef HELIOS_CORE_AVAILABLE
#error "C translation unit received HELIOS_CORE_AVAILABLE"
#endif

int main(void) {
  const HeliosVersion header = helios_header_version();
  const HeliosVersion linked = helios_linked_version();

  if (!helios_versions_compatible(header, linked)) {
    return 1;
  }
  if (!helios_compatible_with_headers()) {
    return 2;
  }
  if (helios_version_string() == 0 || helios_version_string()[0] == '\0') {
    return 3;
  }
  helios_set_last_error("capi");
  if (helios_last_error_message()[0] != 'c') {
    return 4;
  }
  helios_set_last_error(0);
  if (helios_last_error_message()[0] != '\0') {
    return 5;
  }
  return 0;
}
