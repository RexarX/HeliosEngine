#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <helios/log/logger.hpp>

int main(int argc, char* argv[]) {
  doctest::Context context;
  context.applyCommandLine(argc, argv);

  const int result = context.run();

  helios::log::Logger::Instance().FlushAll();

  return result;
}
