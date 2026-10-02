#include <pch.hpp>

#include <helios/version.hpp>

namespace helios {

Version LinkedVersion() noexcept {
  return HeaderVersion();
}

const char* VersionString() noexcept {
  return HELIOS_VERSION_STRING;
}

}  // namespace helios
