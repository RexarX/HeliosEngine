#include "Chunk.h"

namespace VoxelEngine
{
  Chunk::~Chunk()
  {
    delete[] m_Vertices;
    delete[] m_Indices;
  }
}