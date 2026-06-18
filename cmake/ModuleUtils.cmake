# Helios Engine Module Utilities — Facade
#
# Thin compatibility layer that includes the split module files.
# Preserves all existing include(ModuleUtils) call sites.

include_guard(GLOBAL)

include(ModuleRegistry)
include(ModuleDiscovery)
include(ModuleBuilder)
include(ModuleLinking)
