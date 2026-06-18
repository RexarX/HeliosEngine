#pragma once

#include <helios/profile/backend.hpp>
#include <helios/profile/common.hpp>
#include <helios/profile/config.hpp>
#include <helios/profile/frame.hpp>
#include <helios/profile/macros.hpp>
#include <helios/profile/memory.hpp>
#include <helios/profile/message.hpp>
#include <helios/profile/plot.hpp>
#include <helios/profile/profiler.hpp>
#include <helios/profile/zone.hpp>

// Concrete backends are not included here — include them where you call
// AddBackend<>(), e.g. <helios/profile/backends/tracy.hpp>.
// Lock profiling is Tracy-specific — include <helios/profile/tracy/lock.hpp>
// only when needed.
