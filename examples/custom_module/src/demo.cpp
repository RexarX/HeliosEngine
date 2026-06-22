#include <helios/greeting/greeting.hpp>

#include <helios/log/logger.hpp>

int main() {
  helios::log::Info("{}", helios::greeting::Format("Helios"));
  helios::log::Info("{}", helios::greeting::Format(""));
  return 0;
}
