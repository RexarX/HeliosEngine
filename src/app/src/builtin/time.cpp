#include <pch.hpp>

#include <helios/app/builtin/time.hpp>

#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>

namespace helios::app {

void TimePlugin::Build(App& app) {
  app.TryInsertResources(Time{});
  app.AddSystem(kFirst, UpdateTime{});
}

}  // namespace helios::app
