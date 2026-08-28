#pragma once

#include <helios/config.hpp>

#if defined(HELIOS_ENABLE_CPP_MODULES) && defined(HELIOS_BUILDING_MODULE) && \
    !defined(HELIOS_BUILDING_MODULE_CORE)
#else
#include <helios/assert.hpp>
#include <helios/cstring_view.hpp>
#include <helios/delegate.hpp>
#include <helios/stacktrace.hpp>
#include <helios/uuid.hpp>
#endif
