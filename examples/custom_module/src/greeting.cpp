#include <helios/greeting/greeting.hpp>

#include <helios/cstring_view.hpp>

#include <format>
#include <string>

namespace helios::greeting {

std::string Format(helios::CStringView name) {
  if (name.Empty()) {
    return "Hello, Helios!";
  }
  return std::format("Hello, {}!", name);
}

}  // namespace helios::greeting
