project(Imgui)

file(GLOB_RECURSE imgui__src
    "*.h"
    "*.cpp"
)

file(GLOB_RECURSE imgui__backends
    "backends/*.h"
    "backends/*.cpp"
)

file(GLOB_RECURSE imgui__misc
    "misc/cpp/*.h" "misc/cpp/*.cpp"
    "misc/fonts/*.h" "misc/fonts/*.cpp"
)

set(IMGUI 
	${imgui__src}
    ${imgui__backends}
    ${imgui__misc}
)

add_library(imgui STATIC ${IMGUI})

target_include_directories(imgui PUBLIC
    "${CMAKE_INCLUDE_CURRENT_DIR};"
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS imgui)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Dist>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Dist>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(imgui PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_compile_options(imgui PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /fsanitize=address>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        $<$<CONFIG:Dist>:/O2 /Oi /Gy>
        /w
    >
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Debug>:-O0 -g>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Dist>:-O3>
		-w
    >
)

if (ENABLE_SIMD_SSE2)
    target_compile_options(imgui PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
		    $<$<CONFIG:Release>:/arch:SSE2>
		    $<$<CONFIG:Dist>:/arch:SSE2>
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
		    $<$<CONFIG:Release>:-msse2>
		    $<$<CONFIG:Dist>:-msse2>
	    >
    )
elseif (ENABLE_SIMD_SSE3)
    target_compile_options(imgui PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE3
		    $<$<CONFIG:Release>:/arch:SSE2>
		    $<$<CONFIG:Dist>:/arch:SSE2>
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
		    $<$<CONFIG:Release>:-msse3>
		    $<$<CONFIG:Dist>:-msse3>
	    >
    )
elseif (ENABLE_SIMD_SSSE3)
	target_compile_options(imgui PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSSE3
			$<$<CONFIG:Release>:/arch:SSSE3>
			$<$<CONFIG:Dist>:/arch:SSSE3>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-mssse3>
			$<$<CONFIG:Dist>:-mssse3>
		>
	)
elseif (ENABLE_SIMD_SSE4_1)
	target_compile_options(imgui PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.1
			$<$<CONFIG:Release>:/arch:SSE4.1>
			$<$<CONFIG:Dist>:/arch:SSE4.1>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-msse4.1>
			$<$<CONFIG:Dist>:-msse4.1>
		>
	)
elseif (ENABLE_SIMD_SSE4_2)
	target_compile_options(imgui PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.2
			$<$<CONFIG:Release>:/arch:SSE4.2>
			$<$<CONFIG:Dist>:/arch:SSE4.2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-msse4.2>
			$<$<CONFIG:Dist>:-msse4.2>
		>
	)
elseif (ENABLE_SIMD_AVX)
    target_compile_options(imgui PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
			$<$<CONFIG:Release>:/arch:AVX>
			$<$<CONFIG:Dist>:/arch:AVX>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-mavx>
			$<$<CONFIG:Dist>:-mavx>
		>
    )
elseif (ENABLE_SIMD_AVX2)
    target_compile_options(imgui PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
			$<$<CONFIG:Release>:/arch:AVX2>
			$<$<CONFIG:Dist>:/arch:AVX2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:Release>:-mavx2>
			$<$<CONFIG:Dist>:-mavx2>
		>
    )
endif()

find_package(Vulkan REQUIRED)

target_link_libraries(imgui PRIVATE
    glfw
    Vulkan::Vulkan
)