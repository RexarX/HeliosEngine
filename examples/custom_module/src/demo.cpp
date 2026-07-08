#include <helios/greeting/greeting.hpp>
#include <helios/log/log.hpp>

namespace hlog = helios::log;

int main() {
  hlog::Info("{}", helios::greeting::Format("Helios"));
  hlog::Info("{}", helios::greeting::Format(""));
  return 0;
}
