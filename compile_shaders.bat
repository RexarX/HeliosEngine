@echo off

cd VoxelEngine/Assets/Shaders
for /F %%x in ('dir /B/D') do (
  %VK_SDK_PATH%/Bin/glslc.exe %%x -o %%x.spv
)
pause
