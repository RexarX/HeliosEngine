set(PROJECT_NAME VoxelCraft)

set(Sources
    "src/VoxelCraft.cpp"
)

source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Sources}
)

add_executable(${PROJECT_NAME} ${ALL_FILES})

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")

set(ROOT_NAMESPACE VoxelCraft)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG   "VoxelCraft"
    TARGET_NAME_DIST    "VoxelCraft"
    TARGET_NAME_RELEASE "VoxelCraft"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Debug-windows-x86_64/"
    OUTPUT_DIRECTORY_DIST    "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Dist-windows-x86_64/"
    OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../bin/Release-windows-x86_64/"
)

get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)

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
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoxelEngine/vendor/spdlog/include;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoxelEngine/src;"
    "${CMAKE_CURRENT_SOURCE_DIR}/../VoxelEngine/vendor/glm"
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
            /O2;
            /Oi;
            /Gy;
            ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
        >
        $<$<CONFIG:Release>:
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
            /INCREMENTAL
        >
        $<$<CONFIG:Dist>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        $<$<CONFIG:Release>:
            /DEBUG:NONE;
            /OPT:REF;
            /OPT:ICF;
            /INCREMENTAL:NO
        >
        /SUBSYSTEM:CONSOLE
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    VoxelEngine
)