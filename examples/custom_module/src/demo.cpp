#include <helios/greeting/greeting.hpp>

#include <helios/log/logger.hpp>

namespace hlog = helios::log;

int main() {
  hlog::Info("{}", helios::greeting::Format("Helios"));
  hlog::Info("{}", helios::greeting::Format(""));
  return 0;
}
