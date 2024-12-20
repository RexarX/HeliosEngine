project(HeliosEngine)
set(PROJECT_NAME HeliosEngine)

file(GLOB_RECURSE src
    "src/*.h" "src/*.cpp"
)

file(GLOB_RECURSE src__Engine
    "src/HeliosEngine/*.h" "src/HeliosEngine/*.cpp"
)

file(GLOB_RECURSE src__Engine__Config
    "src/HeliosEngine/Config/*.h" "src/HeliosEngine/Config/*.cpp"
)

file(GLOB_RECURSE src__Engine__EntityComponentSystem
    "src/HeliosEngine/EntityComponentSystem/*.h"
    "src/HeliosEngine/EntityComponentSystem/*.inl"
    "src/HeliosEngine/EntityComponentSystem/*.cpp"
    "src/HeliosEngine/EntityComponentSystem/Systems/*.h"
    "src/HeliosEngine/EntityComponentSystem/Systems/*.cpp"
)

file(GLOB_RECURSE src__Engine__Events
    "src/HeliosEngine/Events/*.h" "src/HeliosEngine/Events/*.cpp"
)

file(GLOB_RECURSE src__Engine__ImGui
    "src/HeliosEngine/ImGui/*.h" "src/HeliosEngine/ImGui/*.cpp"
)

file(GLOB_RECURSE src__Engine__Renderer
    "src/HeliosEngine/Renderer/*.h" "src/HeliosEngine/Renderer/*.cpp"
    "src/HeliosEngine/Renderer/Vertex/*.h" "src/HeliosEngine/Renderer/Vertex/*.cpp"
    "src/HeliosEngine/Renderer/Vulkan/*.h" "src/HeliosEngine/Renderer/Vulkan/*.cpp"
    "src/HeliosEngine/Renderer/OpenGL/*.h" "src/HeliosEngine/Renderer/OpenGL/*.cpp"
)

file(GLOB_RECURSE src__Engine__Scene
    "src/HeliosEngine/Scene/*.h" "src/HeliosEngine/Scene/*.cpp"
)

file(GLOB_RECURSE src__Engine__ShaderGraph
    "src/HeliosEngine/ShaderGraph/*.h" "src/HeliosEngine/ShaderGraph/*.cpp"
)

file(GLOB_RECURSE src__Engine__Utils
    "src/HeliosEngine/Utils/*.h" "src/HeliosEngine/Utils/*.cpp"
)

file(GLOB_RECURSE src__Platform__Linux
    "src/Platform/Linux/*.h" "src/Platform/Linux/*.cpp"
)

file(GLOB_RECURSE src__Platform__Windows
    "src/Platform/Windows/*.h" "src/Platform/Windows/*.cpp"
)

set(ALL_FILES
    ${src}
    ${src__Engine}
    ${src__Engine__Config}
    ${src__Engine__EntityComponentSystem}
    ${src__Engine__Events}
    ${src__Engine__ImGui}
    ${src__Engine__Renderer}
    ${src__Engine__Scene}
    ${src__Engine__ShaderGraph}
    ${src__Engine__Utils}
    ${src__Platform__Linux} 
    ${src__Platform__Windows}
)

if(ENABLE_UNITY_BUILD)
    set_source_files_properties(${src__Engine} PROPERTIES UNITY_GROUP "engine")
    set_source_files_properties(${src__Engine__Config} PROPERTIES UNITY_GROUP "config")  
    set_source_files_properties(${src__Engine__EntityComponentSystem} PROPERTIES UNITY_GROUP "ecs")    
    set_source_files_properties(${src__Engine__Events} PROPERTIES UNITY_GROUP "events")
    set_source_files_properties(${src__Engine__ImGui} PROPERTIES UNITY_GROUP "imgui")
    set_source_files_properties(${src__Engine__Renderer} PROPERTIES UNITY_GROUP "renderer")
    set_source_files_properties(${src__Engine__Scene} PROPERTIES UNITY_GROUP "scene")
    set_source_files_properties(${src__Engine__ShaderGraph} PROPERTIES UNITY_GROUP "shader_graph")
    set_source_files_properties(${src__Engine__Utils} PROPERTIES UNITY_GROUP "utils")
    set_source_files_properties(${src__Platform__Linux} PROPERTIES UNITY_GROUP "platform_linux")
    set_source_files_properties(${src__Platform__Windows} PROPERTIES UNITY_GROUP "platform_windows")

    if(CMAKE_PCH_ENABLED)
        set_source_files_properties("src/pch.h" PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
    endif()
endif()

add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(ENABLE_UNITY_BUILD)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        UNITY_BUILD ON
        UNITY_BUILD_MODE BATCH
        CMAKE_UNITY_BUILD_BATCH_SIZE ${UNITY_BUILD_BATCH_SIZE}
    )
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/src/pch.h>"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} UPOUTPUTCONFIG)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_${UPOUTPUTCONFIG} ${PROJECT_NAME}
        ARCHIVE_OUTPUT_NAME_${UPOUTPUTCONFIG} ${PROJECT_NAME}
        RUNTIME_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
        LIBRARY_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
        ARCHIVE_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
    )
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/HeliosEngine"
)

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        "DEBUG_MODE"
    >
    $<$<CONFIG:RelWithDebInfo>:
        "RELEASE_WITH_BEDUG_INFO_MODE"
    >
    $<$<CONFIG:Release>:
        "RELEASE_MODE"
    >
    "BUILD_SHARED"
    "GLFW_INCLUDE_NONE"
    "GLM_FORCE_DEPTH_ZERO_TO_ONE"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<BOOL:ENABLE_PROFILING>:ENABLE_PROFILING>
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_WINDOWS" "UNICODE" "_UNICODE")
elseif(UNIX)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_LINUX")
    target_compile_options(${PROJECT_NAME} PRIVATE -fPIC $<$<CXX_COMPILER_ID:Clang>:-v -stdlib=libc++>)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy /GL>
        $<$<CONFIG:Release>:/O2 /Oi /Gy /GL>
        /W3
    >
    $<$<CXX_COMPILER_ID:Clang,GNU>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:RelWithDebInfo>:-O3 -flto>
        $<$<CONFIG:Release>:-O3 -flto>
    >
)

if(ENABLE_SIMD_SSE2)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>
	    >
	    $<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-msse2>
		    $<$<CONFIG:Release>:-msse2>   
	    >
    )
elseif(ENABLE_SIMD_SSE3)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>   
	    >
	    $<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-msse3>
		    $<$<CONFIG:Release>:-msse3> 
	    >
    )
elseif(ENABLE_SIMD_SSSE3)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSSE3>
			$<$<CONFIG:Release>:/arch:SSSE3>
		>
		$<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-mssse3>
			$<$<CONFIG:Release>:-mssse3>
		>
	)
elseif(ENABLE_SIMD_SSE4_1)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.1
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.1>
			$<$<CONFIG:Release>:/arch:SSE4.1>
		>
		$<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-msse4.1>
			$<$<CONFIG:Release>:-msse4.1>	
		>
	)
elseif(ENABLE_SIMD_SSE4_2)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.2
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.2>
			$<$<CONFIG:Release>:/arch:SSE4.2>
		>
		$<$<CXX_COMPILER_ID:Clang,GNU>:
			$<$<CONFIG:Release>:-msse4.2>
			$<$<CONFIG:RelWithDebInfo>:-msse4.2>
		>
	)
elseif(ENABLE_SIMD_AVX)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX>
			$<$<CONFIG:Release>:/arch:AVX>
		>
		$<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-mavx>
			$<$<CONFIG:Release>:-mavx>
			
		>
    )
elseif(ENABLE_SIMD_AVX2)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
			$<$<CONFIG:Release>:/arch:AVX2>
		>
		$<$<CXX_COMPILER_ID:Clang,GNU>:
            $<$<CONFIG:RelWithDebInfo>:-mavx2>
			$<$<CONFIG:Release>:-mavx2>
		>
    )
endif()

set(GLM_ENABLE_SIMD_SSE2 ENABLE_SIMD_SSE2)
set(GLM_ENABLE_SIMD_SSE3 ENABLE_SIMD_SSE3)
set(GLM_ENABLE_SIMD_SSSE3 ENABLE_SIMD_SSSE3)
set(GLM_ENABLE_SIMD_SSE4_1 ENABLE_SIMD_SSE4_1)
set(GLM_ENABLE_SIMD_SSE4_2 ENABLE_SIMD_SSE4_2)
set(GLM_ENABLE_SIMD_AVX ENABLE_SIMD_AVX)
set(GLM_ENABLE_SIMD_AVX2 ENABLE_SIMD_AVX2)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

find_package(spdlog CONFIG REQUIRED)
find_package(argparse CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(tomlplusplus REQUIRED IMPORTED_TARGET tomlplusplus)
find_package(stduuid CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_reflect CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog_header_only
    argparse::argparse
    PkgConfig::tomlplusplus
    stduuid
    glfw
    glm::glm-header-only
    EnTT::EnTT
    stb_image
    imgui::imgui
    unofficial::shaderc::shaderc
    spirv-cross-core
    spirv-cross-glsl
    spirv-cross-reflect
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator
    glad::glad
)