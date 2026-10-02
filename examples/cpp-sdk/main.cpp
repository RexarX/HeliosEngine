#include <helios/config.hpp>
#include <helios/version.hpp>

#include <cstdio>

int main() {
  std::printf("Helios C++ SDK %s\n", helios::VersionString());
  return 0;
}
