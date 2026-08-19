#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <expected>
#include <limits>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <new>
#include <shared_mutex>
#include <span>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#if defined(HELIOS_MEMORY_USE_MIMALLOC) && !defined(__SANITIZE_ADDRESS__)
#include <mimalloc.h>
#endif
