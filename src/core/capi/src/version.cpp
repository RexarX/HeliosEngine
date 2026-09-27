#include <pch.hpp>

#include <helios/version.h>

#include <helios/version.hpp>

extern "C" {

HeliosVersion helios_linked_version(void) {
  const helios::Version version = helios::LinkedVersion();
  const HeliosVersion out = {
      .major = version.major,
      .minor = version.minor,
      .patch = version.patch,
  };
  return out;
}

const char* helios_version_string(void) {
  return helios::VersionString();
}

}  // extern "C"
