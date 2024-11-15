#pragma once

#include "Core.h"

#include <boost/uuid.hpp>
#include <boost/uuid/uuid_hash.hpp>

namespace Helios {
	class HELIOSENGINE_API UUID {
	public:
		UUID() : m_UUID(boost::uuids::random_generator()()) {}
    UUID(const UUID& other) = default;
		UUID(UUID&& other) noexcept = default;
		~UUID() = default;

		void operator=(const UUID& other) { m_UUID = other.m_UUID; }
		void operator=(UUID&& other) noexcept { m_UUID = std::move(other.m_UUID); }

    inline bool operator==(const UUID& other) const = default;
    inline bool operator!=(const UUID& other) const = default;

    inline std::string ToString() const { return boost::uuids::to_string(m_UUID); }

    inline const boost::uuids::uuid& GetUUID() const { return m_UUID; }

	private:
		boost::uuids::uuid m_UUID{};
	};
}

namespace std {
  template<>
  struct hash<Helios::UUID> {
    inline size_t operator()(const Helios::UUID& uuid) const {
      return boost::uuids::hash_value(uuid.GetUUID());
    }
  };
}