#pragma once

#include "Utils/Random.h"

namespace Helios
{
	class UUID
	{
	public:
		UUID() : m_UUID(Utils::Random::Value<uint64_t>()) {}
    UUID(const UUID& other) : m_UUID(other.m_UUID) {}
    UUID(UUID&& other) : m_UUID(other.m_UUID) { other.m_UUID = 0; }
		UUID(uint64_t uuid) : m_UUID(uuid) {}

		inline operator uint64_t() const { return m_UUID; }

		void operator=(const UUID& other) { m_UUID = other.m_UUID; }
		void operator=(UUID&& other) { m_UUID = other.m_UUID; other.m_UUID = 0; }

	private:
		uint64_t m_UUID;
	};
}