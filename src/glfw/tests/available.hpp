#pragma once

#include <helios/app/app.hpp>
#include <helios/glfw/plugin.hpp>

#include <GLFW/glfw3.h>
#include <doctest/doctest.h>

namespace helios::glfw::test {

[[nodiscard]] inline bool GlfwAvailable() {
  static const bool available = []() -> bool {
    if (glfwInit() != GLFW_TRUE) {
      return false;
    }
    glfwTerminate();
    return true;
  }();
  return available;
}

struct ScopedGlfwShutdown {
  app::App& app;

  ~ScopedGlfwShutdown() { Plugin{}.Destroy(app); }
};

}  // namespace helios::glfw::test

#define HELIOS_SKIP_IF_NO_GLFW()                                               \
  do {                                                                         \
    if (!::helios::glfw::test::GlfwAvailable()) {                              \
      MESSAGE("GLFW is unavailable (no display / glfwInit failed); skipping"); \
      return;                                                                  \
    }                                                                          \
  } while (false)

// GLFW must already be initialized. Use only when shutdown is owned (RAII).
#define HELIOS_SKIP_IF_NO_MONITOR()                                 \
  do {                                                              \
    if (glfwGetPrimaryMonitor() == nullptr) {                       \
      MESSAGE("No GLFW monitor available (headless CI); skipping"); \
      return;                                                       \
    }                                                               \
  } while (false)
