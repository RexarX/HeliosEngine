#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <helios/log/logger.hpp>

#include <cstdio>

int main(int argc, char* argv[]) {
  int result = 0;
  {
    doctest::Context context;
    context.applyCommandLine(argc, argv);
    result = context.run();
  }

  std::fputs("[helios_app_tests] after doctest\n", stderr);
  std::fflush(stderr);

  helios::log::Logger::Instance().Shutdown();

  std::fputs("[helios_app_tests] after Logger::Shutdown\n", stderr);
  std::fflush(stderr);

  return result;
}
