#pragma once

#include <helios/core_pch.hpp>

#include <uuid.h>

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace helios {

/**
 * @brief A class representing a universally unique identifier (UUID).
 */
class Uuid {
public:
  /**
   * @brief Default constructor creates an invalid UUID (all zeros).
   */
  constexpr Uuid() noexcept = default;

  /**
   * @brief Construct a UUID from a string representation.
   * @details If the string is not a valid UUID, the resulting UUID will be invalid.
   * @param str The string representation of the UUID.
   */
  explicit Uuid(std::string_view str);

  /**
   * @brief Construct a UUID from a span of bytes.
   * @details The span must be exactly 16 bytes long; otherwise, the resulting UUID will be invalid.
   * @param bytes A span of bytes representing the UUID.
   */
  explicit Uuid(std::span<const std::byte> bytes);
  Uuid(const Uuid&) = default;
  Uuid(Uuid&&) noexcept = default;
  ~Uuid() = default;

  Uuid& operator=(const Uuid&) = default;
  Uuid& operator=(Uuid&&) noexcept = default;

  /**
   * @brief Generate a new random UUID.
   * @details Uses a thread_local generator for thread-safety without locks.
   * Each thread maintains its own generator state for optimal performance.
   * @return A new random UUID.
   */
  static Uuid Generate();

  /**
   * @brief Convert the UUID to its string representation.
   * @return The string representation of the UUID, or an empty string if the UUID is invalid.
   */
  [[nodiscard]] std::string ToString() const;

  /**
   * @brief Get the UUID as a span of bytes.
   * @return A span of bytes representing the UUID, or an empty span if the UUID is invalid.
   */
  [[nodiscard]] auto AsBytes() const -> std::span<const std::byte>;

  [[nodiscard]] bool operator==(const Uuid& other) const noexcept { return uuid_ == other.uuid_; }
  [[nodiscard]] bool operator!=(const Uuid& other) const noexcept { return !(*this == other); }

  [[nodiscard]] bool operator<(const Uuid& other) const noexcept { return uuid_ < other.uuid_; }

  /**
   * @brief Check if the UUID is valid (not all zeros).
   * @return True if the UUID is valid, false otherwise.
   */
  [[nodiscard]] constexpr bool Valid() const noexcept { return !uuid_.is_nil(); }

  /**
   * @brief Compute a hash value for the UUID.
   * @return A hash value for the UUID.
   */
  [[nodiscard]] size_t Hash() const noexcept { return std::hash<uuids::uuid>{}(uuid_); }

  /**
   * @brief Swap two UUIDs.
   * @param lhs The first UUID.
   * @param rhs The second UUID.
   */
  friend void swap(Uuid& lhs, Uuid& rhs) noexcept { lhs.uuid_.swap(rhs.uuid_); }

private:
  /**
   * @brief Private constructor from uuids::uuid for internal use.
   * @param uuid The uuids::uuid to wrap.
   */
  explicit constexpr Uuid(const uuids::uuid& uuid) noexcept : uuid_(uuid) {}

  uuids::uuid uuid_;

  friend class UuidGenerator;
};

inline Uuid::Uuid(std::string_view str) {
  if (str.empty()) [[unlikely]] {
    return;
  }

  auto result = uuids::uuid::from_string(str.data());
  if (result.has_value()) {
    uuid_ = result.value();
  }
}

inline Uuid::Uuid(std::span<const std::byte> bytes) {
  if (bytes.size() == 16) {
    const auto* begin = std::bit_cast<const uint8_t*>(bytes.data());
    const uint8_t* end = begin + bytes.size();
    uuid_ = uuids::uuid(begin, end);
  }
}

inline Uuid Uuid::Generate() {
  // Create a thread-local engine and pass it (as an lvalue) to the generator.
  // This avoids binding a temporary to a non-const reference.
  static thread_local std::random_device rd;
  static thread_local std::array<std::random_device::result_type, std::mt19937::state_size> seed_data = []() {
    std::array<std::random_device::result_type, std::mt19937::state_size> data = {};
    std::ranges::generate(data, std::ref(rd));
    return data;
  }();
  static thread_local std::seed_seq seq(seed_data.begin(), seed_data.end());
  static thread_local std::mt19937 engine(seq);
  static thread_local uuids::uuid_random_generator gen(engine);
  return Uuid(gen());
}

inline std::string Uuid::ToString() const {
  if (!Valid()) {
    return {};
  }
  return uuids::to_string(uuid_);
}

inline auto Uuid::AsBytes() const -> std::span<const std::byte> {
  if (!Valid()) {
    return {};
  }
  const auto bytes = uuid_.as_bytes();
  return {bytes.data(), bytes.size()};
}

/**
 * @brief A class for generating random UUIDs using a specified random number generator.
 */
class UuidGenerator {
public:
  /**
   * @brief Default constructor initializes the generator with a random seed.
   */
  UuidGenerator() : generator_(std::random_device{}()) {}

  /**
   * @brief Construct a UuidGenerator with a custom random engine.
   * @tparam RandomEngine The type of the random engine.
   * @param engine An instance of the random engine.
   */
  template <typename RandomEngine>
    requires(!std::same_as<std::decay_t<RandomEngine>, UuidGenerator>)
  explicit UuidGenerator(RandomEngine&& engine) : generator_(std::forward<RandomEngine>(engine)) {}
  UuidGenerator(const UuidGenerator&) = delete;
  UuidGenerator(UuidGenerator&&) noexcept = default;
  ~UuidGenerator() = default;

  UuidGenerator& operator=(const UuidGenerator&) = delete;
  UuidGenerator& operator=(UuidGenerator&&) noexcept = default;

  Uuid Generate();

private:
  std::mt19937_64 generator_;
};

inline Uuid UuidGenerator::Generate() {
  uuids::basic_uuid_random_generator<std::mt19937_64> gen(&generator_);
  return Uuid(gen());
}

}  // namespace helios

template <>
struct std::hash<helios::Uuid> {
  size_t operator()(const helios::Uuid& uuid) const noexcept { return uuid.Hash(); }
};
