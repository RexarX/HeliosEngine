set(PROJECT_NAME VoxelCraft)

file(GLOB_RECURSE src
    "src/*.cpp"
)

add_executable(${PROJECT_NAME} ${src})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} UPOUTPUTCONFIG)

    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_${UPOUTPUTCONFIG} "VoxelCraft"
        ARCHIVE_OUTPUT_NAME_${UPOUTPUTCONFIG} "VoxelCraft"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoxelEngine/src;"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    "$<$<CONFIG:Debug>:"
        "DEBUG_MODE"
    ">"
    "$<$<CONFIG:Release>:"
        "RELEASE_MODE"
    ">"
    "$<$<CONFIG:Dist>:"
        "DIST_MODE"
    ">"
    "PLATFORM_WINDOWS;"
    "UNICODE;"
    "_UNICODE"
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /Od;
            /Zi;
        >
        $<$<CONFIG:Release>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        $<$<CONFIG:Dist>:
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        /W3;
        ${DEFAULT_CXX_EXCEPTION_HANDLING};
        /Y-
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:
            /DEBUG;
            /INCREMENTAL;
        >
        $<$<CONFIG:Release>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        $<$<CONFIG:Dist>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO;
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
	VoxelEngine
    imgui
)