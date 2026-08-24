#include <pch.hpp>

#include <helios/app/builtin/frame_count.hpp>

#include <helios/app/application.hpp>
#include <helios/app/schedules.hpp>

namespace helios::app {

void FrameCountPlugin::Build(App& app) {
  app.TryInsertResources(FrameCount{});
  app.AddSystem(kLast, CountFrame{});
}

}  // namespace helios::app
