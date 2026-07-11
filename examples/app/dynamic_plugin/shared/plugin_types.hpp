#pragma once

namespace example_plugins {

// Types that cross the executable/plugin boundary live in a shared header so
// both targets agree on the exact resource type.
struct PluginTicks {
  int count = 0;
};

}  // namespace example_plugins
