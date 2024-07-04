#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <thread>
#include <filesystem>
#include <optional>
#include <cstdint>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "VoxelEngine/Core.h"
#include "VoxelEngine/Log.h"
#include "VoxelEngine/Input.h"
#include "VoxelEngine/KeyCodes.h"
#include "VoxelEngine/MouseButtonCodes.h"

#include "VoxelEngine/Utils/Timer.h"

#ifdef VE_PLATFORM_WINDOWS
  #include <Windows.h>
#endif