#include <pch.hpp>

#include <helios/app/builtin/executor.hpp>

#include <helios/app/application.hpp>

namespace helios::app {

void ExecutorPlugin::Build(App& app) {
  app.TryInsertResources(Executor{app.GetExecutor()});
}

}  // namespace helios::app
