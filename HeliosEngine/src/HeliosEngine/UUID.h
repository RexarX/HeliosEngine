#pragma once

#include "Core.h"

#include <stduuid/uuid.h>

namespace Helios {
	class HELIOSENGINE_API UUID {
	public:
		UUID() : m_UUID(m_Generator()) {}
		UUID(std::string_view string) {
			if (auto result = uuids::uuid::from_string(string)) {
				m_UUID = *result;
			} else {
				m_UUID = m_Generator();
			}
		}

		UUID(const UUID& other) : m_UUID(other.m_UUID) {};
		UUID(UUID&& other) noexcept : m_UUID(std::move(other.m_UUID)) {};
		~UUID() = default;

		UUID& operator=(const UUID& other) {
			m_UUID = other.m_UUID;
			return *this;
		}

		UUID& operator=(UUID&& other) noexcept {
			m_UUID = std::move(other.m_UUID);
			return *this;
		}

		inline bool operator==(const UUID& other) const { return m_UUID == other.m_UUID; }
		inline bool operator!=(const UUID& other) const { return m_UUID != other.m_UUID; }

		inline std::string ToString() const { return uuids::to_string(m_UUID); }
		inline std::span<const std::byte, 16ULL> AsBytes() const { return m_UUID.as_bytes(); }

	private:
		static std::mt19937_64 InitializeEngine() {
			std::random_device device;
			std::array<uint64_t, std::mt19937_64::state_size> seedData;
			std::generate(seedData.begin(), seedData.end(), std::ref(device));
			std::seed_seq seq(seedData.begin(), seedData.end());
			return std::mt19937_64(seq);
		}

	private:
		uuids::uuid m_UUID;
		static inline std::mt19937_64 m_Engine = InitializeEngine();
		static inline auto m_Generator = uuids::basic_uuid_random_generator(m_Engine);

		friend std::hash<UUID>;
	};
}

namespace std {
	template <>
	struct hash<Helios::UUID> {
		inline size_t operator()(const Helios::UUID& uuid) const {
			hash<uuids::uuid> hasher{};
			return hasher(uuid.m_UUID);
		}
	};
}