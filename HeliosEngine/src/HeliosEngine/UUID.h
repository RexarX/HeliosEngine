#pragma once

#include "Core.h"

#include "Utils/Random.h"

namespace Helios
{
	class HELIOSENGINE_API UUID
	{
	public:
		UUID() : m_UUID(Utils::Random::GetValue<uint64_t>()) {}
		UUID(uint64_t uuid) : m_UUID(uuid) {}
    UUID(const UUID& other) : m_UUID(other.m_UUID) {}
    UUID(UUID&& other) noexcept : m_UUID(other.m_UUID) { other.m_UUID = 0; }

		inline operator uint64_t() const { return m_UUID; }

		void operator=(const UUID& other) { m_UUID = other.m_UUID; }
		void operator=(UUID&& other) noexcept { m_UUID = other.m_UUID; other.m_UUID = 0; }

	private:
		uint64_t m_UUID;
	};
}