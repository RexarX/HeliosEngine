#pragma once

#include "Core.h"

#include <string>
#include <fstream>
#include <filesystem>

namespace Helios::Utils
{
	static std::string ReadFileToString(const std::string& filepath)
	{
		CORE_INFO("Reading file '{0}'", filepath);

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
}