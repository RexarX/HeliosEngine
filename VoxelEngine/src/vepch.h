#pragma once

#include <iostream>
#include <memory>
#include <utility>
#include <algorithm>
#include <functional>
#include <thread>
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

#include <filesystem>

#include "VoxelEngine/Log.h"

#include "VoxelEngine/Utils/Timer.h"

#include "VoxelEngine/Input.h"
#include "VoxelEngine/KeyCodes.h"
#include "VoxelEngine/MouseButtonCodes.h"

#ifdef VE_PLATFORM_WINDOWS
  #include <Windows.h>
#endif