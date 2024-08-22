#include "Random.h"

namespace Helios::Utils
{
  std::random_device Random::m_RandomDevice;
  std::mt19937_64 Random::m_Engine(Random::m_RandomDevice());
}