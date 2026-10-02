#include <helios/capi/capi.h>
#include <helios/version.h>

#include <stdio.h>

#ifdef HELIOS_CORE_AVAILABLE
#error "C translation unit received HELIOS_CORE_AVAILABLE"
#endif

int main(void) {
  const HeliosVersion header = helios_header_version();
  const HeliosVersion linked = helios_linked_version();

  if (!helios_compatible_with_headers()) {
    return 1;
  }

  printf("Helios C API %u.%u.%u (%s)\n", linked.major, linked.minor,
         linked.patch, helios_version_string());

  helios_set_last_error("c-sdk example");
  if (helios_last_error_message()[0] != 'c') {
    return 2;
  }
  helios_set_last_error(0);

  (void)header;
  return 0;
}
