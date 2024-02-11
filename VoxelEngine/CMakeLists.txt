set(PROJECT_NAME VoxelEngine)

set(src
    "src/vepch.cpp"
    "src/vepch.h"
    "src/VoxelEngine.h"
)
source_group("src" FILES ${src})

#set(src__Platform__OpenGL
#    "src/Platform/OpenGL/OpenGLBuffer.cpp"
#    "src/Platform/OpenGL/OpenGLBuffer.h"
#    "src/Platform/OpenGL/OpenGLContext.cpp"
#    "src/Platform/OpenGL/OpenGLContext.h"
#    "src/Platform/OpenGL/OpenGLVertexArray.cpp"
#    "src/Platform/OpenGL/OpenGLVertexArray.h"
#)

#source_group("src\\Platform\\OpenGL" FILES ${src__Platform__OpenGL})

set(src__Platform__Windows
    "src/Platform/Windows/WindowsInput.cpp"
    "src/Platform/Windows/WindowsInput.h"
    "src/Platform/Windows/WindowsWindow.cpp"
    "src/Platform/Windows/WindowsWindow.h"
)
source_group("src\\Platform\\Windows" FILES ${src__Platform__Windows})

set(src__VoxelEngine
    "src/VoxelEngine/Application.cpp"
    "src/VoxelEngine/Application.h"
    "src/VoxelEngine/Core.h"
    "src/VoxelEngine/EntryPoint.h"
    "src/VoxelEngine/Input.h"
    "src/VoxelEngine/KeyCodes.h"
    "src/VoxelEngine/Layer.cpp"
    "src/VoxelEngine/Layer.h"
    "src/VoxelEngine/LayerStack.cpp"
    "src/VoxelEngine/LayerStack.h"
    "src/VoxelEngine/Log.cpp"
    "src/VoxelEngine/Log.h"
    "src/VoxelEngine/MouseButtonCodes.h"
    "src/VoxelEngine/Window.h"
)

source_group("src\\VoxelEngine" FILES ${src__VoxelEngine})

set(src__VoxelEngine__Events
    "src/VoxelEngine/Events/ApplicationEvent.h"
    "src/VoxelEngine/Events/Event.h"
    "src/VoxelEngine/Events/KeyEvent.h"
    "src/VoxelEngine/Events/MouseEvent.h"
)
source_group("src\\VoxelEngine\\Events" FILES ${src__VoxelEngine__Events})

#set(src__VoxelEngine__Render
#    "src/VoxelEngine/Render/Buffer.cpp"
#    "src/VoxelEngine/Render/Buffer.h"
#    "src/VoxelEngine/Render/GraphicsContext.h"
#    "src/VoxelEngine/Render/Renderer.cpp"
#    "src/VoxelEngine/Render/Renderer.h"
#    "src/VoxelEngine/Render/Shader.cpp"
#    "src/VoxelEngine/Render/Shader.h"
#    "src/VoxelEngine/Render/VertexArray.cpp"
#    "src/VoxelEngine/Render/VertexArray.h"
#)
#source_group("src\\VoxelEngine\\Render" FILES ${src__VoxelEngine__Render})

set(ALL_FILES
    ${src}
    #${src__Platform__OpenGL}
    ${src__Platform__Windows}
    ${src__VoxelEngine}
    ${src__VoxelEngine__Events}
    #${src__VoxelEngine__Render}
)

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/vepch.h>"
)

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE VoxelEngine)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "VoxelEngine"
    TARGET_NAME_DIST    "VoxelEngine"
    TARGET_NAME_RELEASE "VoxelEngine"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64/VoxelEngine/"
    OUTPUT_DIRECTORY_DIST    "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Dist-windows-x86_64/VoxelEngine/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64/VoxelEngine/"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64/VoxelEngine"
    ARCHIVE_OUTPUT_DIRECTORY_DIST    "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Dist-windows-x86_64/VoxelEngine"
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64/VoxelEngine"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_NAME_DEBUG   "VoxelEngine"
    ARCHIVE_OUTPUT_NAME_DIST    "VoxelEngine"
    ARCHIVE_OUTPUT_NAME_RELEASE "VoxelEngine"
)

get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebug
    >
    $<$<CONFIG:Dist>:
        MultiThreaded
    >
    $<$<CONFIG:Release>:
        MultiThreaded
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Dist>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui;"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "VE_DEBUG"
    ">"
    "$<$<CONFIG:Dist>:"
        "VE_DIST"
    ">"
    "$<$<CONFIG:Release>:"
        "VE_RELEASE"
    ">"
    "VE_PLATFORM_WINDOWS;"
    "VE_BUILD_DLL;"
    "GLFW_INCLUDE_NONE;"
    "UNICODE;"
    "_UNICODE"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /ZI
        >
        $<$<CONFIG:Dist>:
            /Gm-;
            /Ox;
            /Oi;
            /GF;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        $<$<CONFIG:Release>:
            /Gm-;
            /Ox;
            /Oi;
            /GF;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        /W3;
        ${DEFAULT_CXX_EXCEPTION_HANDLING}
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG;
            /INCREMENTAL
        >
        $<$<CONFIG:Dist>:
            /DEBUG:FULL;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:Release>:
            /DEBUG:FULL;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /SUBSYSTEM:WINDOWS
    )
endif()

add_custom_command_if(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMANDS
    COMMAND   $<CONFIG:Debug> IF EXIST ..\\bin\\Debug-windows-x86_64\\VoxelEngine\\VoxelEngine.dll\\ (xcopy /Q /E /Y /I ..\\bin\\Debug-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Debug-windows-x86_64\\VoxelCraft > nul) ELSE (xcopy /Q /Y /I ..\\bin\\Debug-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Debug-windows-x86_64\\VoxelCraft > nul)
    COMMAND    $<CONFIG:Dist> IF EXIST ..\\bin\\Dist-windows-x86_64\\VoxelEngine\\VoxelEngine.dll\\ (xcopy /Q /E /Y /I ..\\bin\\Dist-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Dist-windows-x86_64\\VoxelCraft > nul) ELSE (xcopy /Q /Y /I ..\\bin\\Dist-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Dist-windows-x86_64\\VoxelCraft > nul)
    COMMAND $<CONFIG:Release> IF EXIST ..\\bin\\Release-windows-x86_64\\VoxelEngine\\VoxelEngine.dll\\ (xcopy /Q /E /Y /I ..\\bin\\Release-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Release-windows-x86_64\\VoxelCraft > nul) ELSE (xcopy /Q /Y /I ..\\bin\\Release-windows-x86_64\\VoxelEngine\\VoxelEngine.dll ..\\bin\\Release-windows-x86_64\\VoxelCraft > nul)
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw
    glad
    glm
	spdlog
)

set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "opengl32"
)

target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")