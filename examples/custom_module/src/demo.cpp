#include <helios/greeting/greeting.hpp>
#include <helios/log/log.hpp>

namespace hlog = helios::log;

int main() {
  // The demo executable links against the out-of-tree greeting module just like
  // it would link against a built-in Helios module.
  hlog::Info("{}", helios::greeting::Format("Helios"));

  // An empty name exercises the module's fallback behavior.
  hlog::Info("{}", helios::greeting::Format(""));
  return 0;
}
