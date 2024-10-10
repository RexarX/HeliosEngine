project(ObjLoader)

file(GLOB_RECURSE src
    "*.h"
    "*.cpp"
)

add_library(objloader STATIC ${src})

target_include_directories(objloader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS objloader)

target_compile_options(objloader PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /fsanitize=address>
		$<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Gy>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        /w
    >
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Debug>:-O0 -g>
		$<$<CONFIG:RelWithDebInfo>:-O3>
        $<$<CONFIG:Release>:-O3>
		-w
    >
)

if (ENABLE_SIMD_SSE2)
    target_compile_options(objloader PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse2>
		    $<$<CONFIG:Release>:-msse2>   
	    >
    )
elseif (ENABLE_SIMD_SSE3)
    target_compile_options(objloader PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE2>
		    $<$<CONFIG:Release>:/arch:SSE2>   
	    >
	    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse3>
		    $<$<CONFIG:Release>:-msse3> 
	    >
    )
elseif (ENABLE_SIMD_SSSE3)
	target_compile_options(objloader PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSSE3
            $<$<CONFIG:RelWithDebInfo>:/arch:SSSE3>
			$<$<CONFIG:Release>:/arch:SSSE3>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mssse3>
			$<$<CONFIG:Release>:-mssse3>
		>
	)
elseif (ENABLE_SIMD_SSE4_1)
	target_compile_options(objloader PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.1
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.1>
			$<$<CONFIG:Release>:/arch:SSE4.1>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-msse4.1>
			$<$<CONFIG:Release>:-msse4.1>	
		>
	)
elseif (ENABLE_SIMD_SSE4_2)
	target_compile_options(objloader PRIVATE
		$<$<CXX_COMPILER_ID:MSVC>: # VC doesn't support SSE4.2
            $<$<CONFIG:RelWithDebInfo>:/arch:SSE4.2>
			$<$<CONFIG:Release>:/arch:SSE4.2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
			$<$<CONFIG:RelWithDebInfo>:-msse4.2>
			$<$<CONFIG:Release>:-msse4.2>
		>
	)
elseif (ENABLE_SIMD_AVX)
    target_compile_options(objloader PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX>
			$<$<CONFIG:Release>:/arch:AVX>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mavx>
			$<$<CONFIG:Release>:-mavx>
			
		>
    )
elseif (ENABLE_SIMD_AVX2)
    target_compile_options(objloader PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:
            $<$<CONFIG:RelWithDebInfo>:/arch:AVX2>
			$<$<CONFIG:Release>:/arch:AVX2>
		>
		$<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
            $<$<CONFIG:RelWithDebInfo>:-mavx2>
			$<$<CONFIG:Release>:-mavx2>
		>
    )
endif()