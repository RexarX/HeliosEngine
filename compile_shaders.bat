@echo off

cd VoxelEngine/Assets/Shaders
for /F %%x in ('dir /B/D') do (
  %VK_SDK_PATH%/Bin/glslc.exe %%x -o %%x.spv
  %VK_SDK_PATH%/Bin/spirv-opt.exe %%x.spv -o %%x.spv
)
pause
