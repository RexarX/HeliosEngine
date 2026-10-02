#include <helios/core/core.hpp>
#include <helios/greeting/greeting.hpp>

#include <format>
#include <string>

namespace helios::greeting {

std::string Format(helios::CStringView name) {
  // Keep the public function total: callers get a friendly default instead of
  // having to special-case an empty name themselves.
  if (name.Empty()) {
    return "Hello, Helios!";
  }
  return std::format("Hello, {}!", name);
}

}  // namespace helios::greeting
