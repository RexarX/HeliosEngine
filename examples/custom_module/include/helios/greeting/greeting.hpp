#pragma once

#include <helios/core/core.hpp>

#include <string>

namespace helios::greeting {

/// @brief Formats a greeting for the given name.
[[nodiscard]] std::string Format(helios::CStringView name);

}  // namespace helios::greeting
