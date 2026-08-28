#include <pch.hpp>

#include <helios/app/plugin_group.hpp>

#include <helios/app/application.hpp>

#include <helios/assert.hpp>
#include <ranges>
#include <utility>

namespace helios::app {

void PluginGroup::Build(App& app) {
  for (auto& storage : plugins_ | std::views::values) {
    if (storage.disabled) [[unlikely]] {
      continue;
    }

    HELIOS_ASSERT(storage.plugin != nullptr,
                  "Enabled plugin storage has no plugin instance!");
    if (storage.plugin == nullptr) [[unlikely]] {
      continue;
    }

    app.AddPlugin(storage.id, std::move(storage.plugin));
  }
}

}  // namespace helios::app
