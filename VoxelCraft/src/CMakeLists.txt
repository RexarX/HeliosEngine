add_executable(VoxelCraft VoxelCraft.cpp)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT VoxelCraft)

target_link_libraries(VoxelCraft VoxelEngine)