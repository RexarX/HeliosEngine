#pragma once

#include <helios/core_pch.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>

namespace helios::utils {

enum class FileError : uint8_t { kCouldNotOpen, kReadError };

/**
 * @brief Converts FileError to a human-readable string.
 * @param error The FileError to convert
 * @return A string view representing the error
 */
[[nodiscard]] constexpr std::string_view FileErrorToString(FileError error) noexcept {
  switch (error) {
    case FileError::kCouldNotOpen:
      return "Could not open file";
    case FileError::kReadError:
      return "Could not read file";
    default:
      return "Unknown file error";
  }
}

/**
 * @brief Reads the entire contents of a file into a string.
 * @param filepath The path to the file
 * @return An expected containing the file contents or a FileError
 */
[[nodiscard]] inline auto ReadFileToString(std::string_view filepath) -> std::expected<std::string, FileError> {
  if (filepath.empty()) {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ifstream in(filepath.data(), std::ios::in | std::ios::binary);
  if (!in) {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::string result;
  in.seekg(0, std::ios::end);
  const auto pos = in.tellg();
  if (pos == std::ifstream::pos_type(-1)) {
    return std::unexpected(FileError::kReadError);
  }

  const auto size = static_cast<size_t>(pos);
  result.resize(size);
  in.seekg(0, std::ios::beg);
  in.read(result.data(), static_cast<std::streamsize>(result.size()));
  if (!in) {
    return std::unexpected(FileError::kReadError);
  }
  in.close();

  return result;
}

/**
 * @brief Reads the entire contents of a file into a string.
 * @param filepath The path to the file
 * @return An expected containing the file contents or a FileError
 */
[[nodiscard]] inline auto ReadFileToString(const std::filesystem::path& filepath)
    -> std::expected<std::string, FileError> {
  if (filepath.empty()) {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::ifstream in(filepath, std::ios::in | std::ios::binary);
  if (!in) {
    return std::unexpected(FileError::kCouldNotOpen);
  }

  std::string result;
  in.seekg(0, std::ios::end);
  const auto pos = in.tellg();
  if (pos == std::ifstream::pos_type(-1)) {
    return std::unexpected(FileError::kReadError);
  }

  const auto size = static_cast<size_t>(pos);
  result.resize(size);
  in.seekg(0, std::ios::beg);
  in.read(result.data(), static_cast<std::streamsize>(result.size()));
  if (!in) {
    return std::unexpected(FileError::kReadError);
  }
  in.close();

  return result;
}

/**
 * @brief Extracts the file name from a given path.
 * @param path The full file path
 * @return The file name
 */
[[nodiscard]] constexpr std::string_view GetFileName(std::string_view path) {
  const size_t last_slash = path.find_last_of("/\\");
  return (last_slash != std::string_view::npos) ? path.substr(last_slash + 1) : path;
}

/**
 * @brief Extracts the file extension from a given path.
 * @param path The full file path
 * @return The file extension, including the dot (e.g., ".txt"), or an empty string if none exists
 */
[[nodiscard]] static constexpr std::string_view GetFileExtension(std::string_view path) {
  const size_t last_dot = path.find_last_of('.');
  return (last_dot != std::string_view::npos) ? path.substr(last_dot) : "";
}

}  // namespace helios::utils
