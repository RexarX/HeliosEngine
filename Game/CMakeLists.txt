set(PROJECT_NAME Game)

file(GLOB_RECURSE src
    "src/*.h" "src/*.cpp"
)

file(GLOB_RECURSE src__Layers
    "src/Layers/*.h" "src/Layers/*.cpp"
)

file(GLOB V_GLOB LIST_DIRECTORIES true "src/Scripts/*")
foreach(item ${V_GLOB})
	if(IS_DIRECTORY ${item})
        string(CONCAT header_files ${item} "/*.h")
        string(CONCAT cpp_files ${item} "/*.cpp")
		list(APPEND Scripts ${header_files} " " ${cpp_files})
        message("Found scripts: " ${Scripts})
	endif()
endforeach()

file(GLOB_RECURSE src__Scripts
	"src/Scripts/*.h" "src/Scripts/*.cpp"
	
	${Scripts}
)

set(ALL_FILES ${src} ${src__Layers} ${src__Scripts})

add_executable(${PROJECT_NAME} ${ALL_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} UPOUTPUTCONFIG)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_${UPOUTPUTCONFIG} "Game"
        ARCHIVE_OUTPUT_NAME_${UPOUTPUTCONFIG} "Game"
        RUNTIME_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
        ARCHIVE_OUTPUT_DIRECTORY_${UPOUTPUTCONFIG}
            "${CMAKE_CURRENT_SOURCE_DIR}/../bin/${OUTPUTCONFIG}-${PLATFORM}-${ARCHITECTURE}"
    )
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)

string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
    $<$<CONFIG:Debug>:
        MultiThreadedDebugDLL
    >
    $<$<CONFIG:Release>:
        MultiThreadedDLL
    >
    $<$<CONFIG:Dist>:
        MultiThreadedDLL
    >
    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>,$<CONFIG:Dist>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
)
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../HeliosEngine/src;"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:
        "DEBUG_MODE;"
    >
    $<$<CONFIG:Release>:
        "RELEASE_MODE;"
    >
    $<$<CONFIG:Dist>:
        "DIST_MODE;"
    >
    "UNICODE;"
    "_UNICODE;"
)

if(WIN32)
	target_compile_definitions(${PROJECT_NAME} PRIVATE "PLATFORM_WINDOWS;")
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:
        $<$<CONFIG:Debug>:/Od /Zi /fsanitize=address>
        $<$<CONFIG:Release>:/O2 /Oi /Gy>
        $<$<CONFIG:Dist>:/O2 /Oi /Gy>
        /W3
    >
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
        $<$<CONFIG:Debug>:-O0 -g -fsanitize=address>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Dist>:-O3>
        -Wno-switch
    >
)

if (ENABLE_SIMD_SSE2)
    target_compile_options(${PROJECT_NAME} PRIVATE
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
    target_compile_options(${PROJECT_NAME} PRIVATE
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
	target_compile_options(${PROJECT_NAME} PRIVATE
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
	target_compile_options(${PROJECT_NAME} PRIVATE
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
	target_compile_options(${PROJECT_NAME} PRIVATE
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
    target_compile_options(${PROJECT_NAME} PRIVATE
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
    target_compile_options(${PROJECT_NAME} PRIVATE
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

target_link_libraries(${PROJECT_NAME} PRIVATE
	HeliosEngine
    imgui
)