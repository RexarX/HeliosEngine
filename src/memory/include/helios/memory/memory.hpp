#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.memory;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#include <helios/memory/aligned_alloc.hpp>
#include <helios/memory/allocator_traits.hpp>
#include <helios/memory/arena_allocator.hpp>
#include <helios/memory/common.hpp>
#include <helios/memory/fixed_arena_allocator.hpp>
#include <helios/memory/fixed_free_list_allocator.hpp>
#include <helios/memory/fixed_pool_allocator.hpp>
#include <helios/memory/fixed_stack_allocator.hpp>
#include <helios/memory/frame_allocator.hpp>
#include <helios/memory/free_list_allocator.hpp>
#include <helios/memory/pool_allocator.hpp>
#include <helios/memory/ref_counted.hpp>
#include <helios/memory/stack_allocator.hpp>
#include <helios/memory/temporary_storage.hpp>
#include <helios/memory/temporary_storage_helpers.hpp>
#include <helios/memory/treiber_stack.hpp>
#endif  // HELIOS_MODULE_CONSUMER_SHIM
