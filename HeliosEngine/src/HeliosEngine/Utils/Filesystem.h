#pragma once

#include "Core.h"

#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>

namespace Helios::Utils {
	static std::string readFileToString(const std::string& filepath) {
		CORE_INFO("Reading file '{}'", filepath);

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);

		if (!in) { CORE_ERROR("Could not open file '{0}'!", filepath); return result; }

		in.seekg(0, std::ios::end);
		result.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(result.data(), result.size());
		in.close();

		return result;
	}

	static inline std::string_view getFileName(std::string_view path) {
		uint64_t lastSlash = path.find_last_of("/\\");
		return (lastSlash != std::string_view::npos) ? path.substr(lastSlash + 1) : path;
	}
}