#include <pch.hpp>

#include <helios/glfw/systems/init.hpp>

#include <helios/assert.hpp>
#include <helios/ecs/resource/params.hpp>
#include <helios/ecs/world.hpp>
#include <helios/glfw/details/glfw_sync.hpp>
#include <helios/log/log.hpp>

#include <GLFW/glfw3.h>

namespace helios::glfw {

namespace {

void ErrorCallback(int error_code, const char* description) {
  log::Error("GLFW error {}: {}", error_code,
             description != nullptr ? description : "unknown error");
}

}  // namespace

void RegisterErrorCallback() {
  glfwSetErrorCallback(ErrorCallback);
}

void Init::operator()(ecs::World& world, ecs::Res<Context> context,
                      ecs::Res<window::Monitors> monitors) const {
  HELIOS_VERIFY(!context->initialized, "GLFW already initialized!");
  RegisterErrorCallback();
  HELIOS_VERIFY(glfwInit() == GLFW_TRUE, "Failed to initialize GLFW!");
  context->initialized = true;
  context->world = &world;

  RegisterMonitorCallback(world);
  RefreshMonitors(*monitors);
}

}  // namespace helios::glfw
