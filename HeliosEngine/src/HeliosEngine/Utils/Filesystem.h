#pragma once

#include "Core.h"

#include <string>
#include <string_view>
#include <fstream>
#include <filesystem>
#include <expected>

namespace Helios::Utils {
	static std::expected<std::string, const char*> readFileToString(std::string_view filepath) {
		std::ifstream in(filepath.data(), std::ios::in | std::ios::binary);
		if (!in) {
			return std::unexpected("Could not open file");
		}

		std::string result;
		in.seekg(0, std::ios::end);
		result.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(result.data(), result.size());
		in.close();

		return result;
	}

	static std::expected<std::string, const char*> readFileToString(std::filesystem::path& filepath) {
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (!in) {
			return std::unexpected("Could not open file");
		}

		std::string result;
		in.seekg(0, std::ios::end);
		result.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(result.data(), result.size());
		in.close();

		return result;
	}

	static inline std::string_view getFileName(std::string_view path) {
		uint64_t lastSlash = path.find_last_of("/\\");
		return (lastSlash != std::string_view::npos) ? path.substr(++lastSlash) : path;
	}

	/* Returns ".extension" */
	static inline std::string_view getFileExtension(std::string_view path) {
		uint64_t lastDot = path.find_last_of('.');
		if (lastDot == std::string_view::npos) {
			return "";
		}
		return path.substr(lastDot);
	}
}