/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Helios Engine", "index.html", [
    [ "Helios Engine", "index.html#helios-engine", [
      [ "Table of Contents", "index.html#table-of-contents", null ],
      [ "About The Project", "index.html#about-the-project", [
        [ "Key Features", "index.html#key-features", null ],
        [ "Design Philosophy", "index.html#design-philosophy", null ]
      ] ],
      [ "Modules", "index.html#modules", null ],
      [ "Getting Started", "index.html#getting-started", [
        [ "Requirements", "index.html#requirements", null ],
        [ "Installing Dependencies", "index.html#installing-dependencies", [
          [ "All platforms — build tools", "index.html#all-platforms--build-tools", null ],
          [ "Linux (APT — Ubuntu / Debian)", "index.html#linux-apt--ubuntu--debian", null ],
          [ "Linux (DNF — Fedora)", "index.html#linux-dnf--fedora", null ],
          [ "Linux (Pacman — Arch)", "index.html#linux-pacman--arch", null ],
          [ "macOS (Homebrew)", "index.html#macos-homebrew", null ],
          [ "Windows (MSVC)", "index.html#windows-msvc", null ]
        ] ],
        [ "Building", "index.html#building-by-platform", [
          [ "Linux (GCC)", "index.html#linux-gcc", null ],
          [ "Linux (Clang)", "index.html#linux-clang", null ],
          [ "Windows", "index.html#windows", null ],
          [ "macOS (Clang)", "index.html#macos-clang", null ],
          [ "Recommended developer flags", "index.html#recommended-developer-flags", null ]
        ] ],
        [ "Run the Example", "index.html#run-the-example", null ]
      ] ],
      [ "Architecture", "index.html#architecture", null ],
      [ "Using as a Dependency", "index.html#using-as-a-dependency", [
        [ "Method 1: <tt>add_subdirectory</tt>", "index.html#method-1-add_subdirectory", null ],
        [ "Method 2: <tt>FetchContent</tt>", "index.html#method-2-fetchcontent", null ],
        [ "Method 3: CPM", "index.html#method-3-cpm", null ],
        [ "Method 4: Installed Package (<tt>find_package</tt>)", "index.html#method-4-installed-package-find_package", null ]
      ] ],
      [ "Documentation", "index.html#documentation", [
        [ "API reference (Doxygen)", "index.html#api-reference-doxygen", null ],
        [ "Project guidelines", "index.html#project-guidelines", null ]
      ] ],
      [ "Development", "index.html#development", [
        [ "Code formatting", "index.html#code-formatting", null ],
        [ "Creating a Custom Module", "index.html#creating-a-custom-module", null ],
        [ "Other scripts", "index.html#other-scripts", null ]
      ] ],
      [ "Roadmap", "index.html#roadmap", null ],
      [ "Acknowledgments", "index.html#acknowledgments", null ],
      [ "License", "index.html#license", null ],
      [ "Contact", "index.html#contact", null ]
    ] ],
    [ "<tt>app</tt> — Application Framework", "md_src_2app_2README.html", [
      [ "Public API", "md_src_2app_2README.html#public-api", [
        [ "Builtin Stages & Schedules (<tt>schedules.hpp</tt>)", "md_src_2app_2README.html#builtin-stages--schedules-scheduleshpp", null ],
        [ "Sub-App Label Traits", "md_src_2app_2README.html#sub-app-label-traits", null ]
      ] ],
      [ "Quick Start", "md_src_2app_2README.html#quick-start", null ],
      [ "Lifecycle", "md_src_2app_2README.html#lifecycle", null ],
      [ "Frame Lifecycle", "md_src_2app_2README.html#frame-lifecycle", null ],
      [ "Sub-Apps", "md_src_2app_2README.html#sub-apps", null ],
      [ "Plugins", "md_src_2app_2README.html#plugins", null ],
      [ "Custom Runner", "md_src_2app_2README.html#custom-runner", null ],
      [ "Dependencies", "md_src_2app_2README.html#dependencies", null ]
    ] ],
    [ "<tt>async</tt> — Task-Based Parallelism", "md_src_2async_2README.html", [
      [ "Public API", "md_src_2async_2README.html#public-api-1", [
        [ "<tt>Executor</tt> Methods", "md_src_2async_2README.html#executor-methods", null ]
      ] ],
      [ "Quick Start", "md_src_2async_2README.html#quick-start-1", null ],
      [ "Parallel Algorithms", "md_src_2async_2README.html#parallel-algorithms", null ],
      [ "Independent Async Tasks", "md_src_2async_2README.html#independent-async-tasks", null ],
      [ "Dynamic Subflows", "md_src_2async_2README.html#dynamic-subflows", null ],
      [ "Dispatch Paths", "md_src_2async_2README.html#dispatch-paths", null ],
      [ "Thread Safety", "md_src_2async_2README.html#thread-safety", null ],
      [ "Dependencies", "md_src_2async_2README.html#dependencies-1", null ]
    ] ],
    [ "<tt>compiler</tt> — Compiler Detection & Intrinsics", "md_src_2compiler_2README.html", [
      [ "Public API", "md_src_2compiler_2README.html#public-api-2", [
        [ "Usage", "md_src_2compiler_2README.html#usage", null ]
      ] ],
      [ "Macros", "md_src_2compiler_2README.html#macros", null ]
    ] ],
    [ "<tt>container</tt> — Data-Oriented Containers", "md_src_2container_2README.html", [
      [ "Public API", "md_src_2container_2README.html#public-api-3", null ],
      [ "SparseSet", "md_src_2container_2README.html#sparseset", null ],
      [ "MultiTypeMap", "md_src_2container_2README.html#multitypemap", null ],
      [ "TypedBuffer", "md_src_2container_2README.html#typedbuffer", null ],
      [ "CallableBuffer", "md_src_2container_2README.html#callablebuffer", null ],
      [ "StaticString", "md_src_2container_2README.html#staticstring", null ],
      [ "Dependencies", "md_src_2container_2README.html#dependencies-2", null ]
    ] ],
    [ "<tt>core</tt> — Core Engine Primitives", "md_src_2core_2README.html", [
      [ "Public API", "md_src_2core_2README.html#public-api-4", null ],
      [ "Assert System", "md_src_2core_2README.html#assert-system", null ],
      [ "UUID", "md_src_2core_2README.html#uuid", null ],
      [ "Stacktrace", "md_src_2core_2README.html#stacktrace", null ],
      [ "CStringView", "md_src_2core_2README.html#cstringview", null ],
      [ "Dependencies", "md_src_2core_2README.html#dependencies-3", null ]
    ] ],
    [ "<tt>ecs</tt> — Entity Component System", "md_src_2ecs_2README.html", [
      [ "Public API", "md_src_2ecs_2README.html#public-api-5", [
        [ "Core Types", "md_src_2ecs_2README.html#core-types", null ],
        [ "World Lifecycle", "md_src_2ecs_2README.html#world-lifecycle", null ]
      ] ],
      [ "Quick Start", "md_src_2ecs_2README.html#quick-start-2", null ],
      [ "Systems & Schedules", "md_src_2ecs_2README.html#systems--schedules", [
        [ "System Parameters", "md_src_2ecs_2README.html#system-parameters", null ],
        [ "Custom System Parameters", "md_src_2ecs_2README.html#custom-system-parameters", [
          [ "Aggregate parameters (<tt>CompositeSystemParam</tt>)", "md_src_2ecs_2README.html#aggregate-parameters-compositesystemparam", null ],
          [ "Fully custom parameters", "md_src_2ecs_2README.html#fully-custom-parameters", null ]
        ] ],
        [ "Ordering", "md_src_2ecs_2README.html#ordering", null ]
      ] ],
      [ "Commands — Deferred Mutations", "md_src_2ecs_2README.html#commands--deferred-mutations", null ],
      [ "Messages", "md_src_2ecs_2README.html#messages", null ],
      [ "Components", "md_src_2ecs_2README.html#components", null ],
      [ "Queries", "md_src_2ecs_2README.html#queries", null ],
      [ "Executors", "md_src_2ecs_2README.html#executors", null ],
      [ "Builtin Types", "md_src_2ecs_2README.html#builtin-types", null ],
      [ "Dependencies", "md_src_2ecs_2README.html#dependencies-4", null ]
    ] ],
    [ "<tt>log</tt> — Logging", "md_src_2log_2README.html", [
      [ "Public API", "md_src_2log_2README.html#public-api-6", [
        [ "Free Functions", "md_src_2log_2README.html#free-functions", null ]
      ] ],
      [ "Quick Start", "md_src_2log_2README.html#quick-start-3", null ],
      [ "Custom Logger Types", "md_src_2log_2README.html#custom-logger-types", null ],
      [ "Configuration", "md_src_2log_2README.html#configuration", null ],
      [ "Runtime Control", "md_src_2log_2README.html#runtime-control", null ],
      [ "Assert Integration", "md_src_2log_2README.html#assert-integration", null ],
      [ "Dependencies", "md_src_2log_2README.html#dependencies-5", null ]
    ] ],
    [ "<tt>memory</tt> — PMR Allocators & Reference Counting", "md_src_2memory_2README.html", [
      [ "Public API", "md_src_2memory_2README.html#public-api-7", [
        [ "Allocators", "md_src_2memory_2README.html#allocators", null ],
        [ "Reference Counting", "md_src_2memory_2README.html#reference-counting", null ],
        [ "Utilities", "md_src_2memory_2README.html#utilities", null ]
      ] ],
      [ "Arena Allocator", "md_src_2memory_2README.html#arena-allocator", null ],
      [ "Frame Allocator", "md_src_2memory_2README.html#frame-allocator", null ],
      [ "Pool Allocator", "md_src_2memory_2README.html#pool-allocator", null ],
      [ "Reference Counting", "md_src_2memory_2README.html#reference-counting-1", null ],
      [ "Dependencies", "md_src_2memory_2README.html#dependencies-6", null ]
    ] ],
    [ "<tt>platform</tt> — Platform Detection & API Macros", "md_src_2platform_2README.html", [
      [ "Public API", "md_src_2platform_2README.html#public-api-8", null ],
      [ "Usage", "md_src_2platform_2README.html#usage-1", null ],
      [ "Macros", "md_src_2platform_2README.html#macros-1", null ]
    ] ],
    [ "<tt>profile</tt> — CPU/Memory/Lock Profiling", "md_src_2profile_2README.html", [
      [ "Public API", "md_src_2profile_2README.html#public-api-9", [
        [ "Free Functions", "md_src_2profile_2README.html#free-functions-1", null ]
      ] ],
      [ "Quick Start", "md_src_2profile_2README.html#quick-start-4", null ],
      [ "Macro Reference", "md_src_2profile_2README.html#macro-reference", [
        [ "Memory in Tracy", "md_src_2profile_2README.html#memory-in-tracy", null ]
      ] ],
      [ "Multi-Backend", "md_src_2profile_2README.html#multi-backend", null ],
      [ "Lock Profiling (Tracy-specific)", "md_src_2profile_2README.html#lock-profiling-tracy-specific", null ],
      [ "Custom Backends", "md_src_2profile_2README.html#custom-backends", null ],
      [ "Build", "md_src_2profile_2README.html#build", null ],
      [ "Thread Safety", "md_src_2profile_2README.html#thread-safety-1", null ],
      [ "Dependencies", "md_src_2profile_2README.html#dependencies-7", null ]
    ] ],
    [ "<tt>utils</tt> — Common Utilities", "md_src_2utils_2README.html", [
      [ "Public API", "md_src_2utils_2README.html#public-api-10", null ],
      [ "Type Identification", "md_src_2utils_2README.html#type-identification", null ],
      [ "Delegate", "md_src_2utils_2README.html#delegate", null ],
      [ "Scope Guard", "md_src_2utils_2README.html#scope-guard", null ],
      [ "Timer", "md_src_2utils_2README.html#timer", null ],
      [ "Random", "md_src_2utils_2README.html#random", null ],
      [ "Filesystem", "md_src_2utils_2README.html#filesystem", null ],
      [ "Dynamic Library", "md_src_2utils_2README.html#dynamic-library", null ],
      [ "Iterator Adapters", "md_src_2utils_2README.html#iterator-adapters", null ],
      [ "Dependencies", "md_src_2utils_2README.html#dependencies-8", null ]
    ] ],
    [ "<tt>window</tt> — Cross-Platform Window Management", "md_src_2window_2README.html", [
      [ "Public API", "md_src_2window_2README.html#public-api-11", null ],
      [ "Intended Scope", "md_src_2window_2README.html#intended-scope", null ],
      [ "Dependencies", "md_src_2window_2README.html#dependencies-9", null ]
    ] ],
    [ "Project Guidelines", "md_docs_2guidelines.html", [
      [ "Code rules", "md_docs_2guidelines.html#code-rules", [
        [ "Non-negotiable", "md_docs_2guidelines.html#non-negotiable", null ],
        [ "Types and APIs", "md_docs_2guidelines.html#types-and-apis", null ],
        [ "Naming", "md_docs_2guidelines.html#naming", null ]
      ] ],
      [ "Class and struct layout", "md_docs_2guidelines.html#class-and-struct-layout", null ],
      [ "Method implementation", "md_docs_2guidelines.html#method-implementation", null ],
      [ "Documentation", "md_docs_2guidelines.html#documentation-1", [
        [ "Tag order", "md_docs_2guidelines.html#tag-order", null ]
      ] ],
      [ "Module structure", "md_docs_2guidelines.html#module-structure", [
        [ "Registration (<tt>Module.cmake</tt>)", "md_docs_2guidelines.html#registration-modulecmake", null ],
        [ "Build definition (<tt>CMakeLists.txt</tt>)", "md_docs_2guidelines.html#build-definition-cmakeliststxt", null ],
        [ "Selective module builds", "md_docs_2guidelines.html#selective-module-builds", null ]
      ] ],
      [ "Testing", "md_docs_2guidelines.html#testing", [
        [ "Structure", "md_docs_2guidelines.html#structure", null ],
        [ "Check macros", "md_docs_2guidelines.html#check-macros", null ],
        [ "Running tests", "md_docs_2guidelines.html#running-tests", null ]
      ] ],
      [ "Building", "md_docs_2guidelines.html#building", [
        [ "Prerequisites", "md_docs_2guidelines.html#prerequisites", null ],
        [ "CMake presets", "md_docs_2guidelines.html#cmake-presets", null ]
      ] ],
      [ "Developer scripts", "md_docs_2guidelines.html#developer-scripts", [
        [ "Formatting enforcement", "md_docs_2guidelines.html#formatting-enforcement", null ]
      ] ],
      [ "Further reading", "md_docs_2guidelines.html#further-reading", null ]
    ] ],
    [ "Creating a Custom Helios Module", "md_examples_2custom__module_2README.html", [
      [ "What you get", "md_examples_2custom__module_2README.html#what-you-get", null ],
      [ "Layout", "md_examples_2custom__module_2README.html#layout", null ],
      [ "Step 1 — Register the module (<tt>Module.cmake</tt>)", "md_examples_2custom__module_2README.html#step-1--register-the-module-modulecmake", null ],
      [ "Step 2 — Define the build (<tt>CMakeLists.txt</tt>)", "md_examples_2custom__module_2README.html#step-2--define-the-build-cmakeliststxt", null ],
      [ "Step 3 — Discovery (no manual <tt>include()</tt>)", "md_examples_2custom__module_2README.html#step-3--discovery-no-manual-include", null ],
      [ "Step 4 — Public API", "md_examples_2custom__module_2README.html#step-4--public-api", null ],
      [ "Step 5 — Consume from an executable", "md_examples_2custom__module_2README.html#step-5--consume-from-an-executable", null ],
      [ "Further reading", "md_examples_2custom__module_2README.html#further-reading-1", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", "namespacemembers_func" ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Concepts", "concepts.html", "concepts" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ],
        [ "Typedefs", "functions_type.html", "functions_type" ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Functions", "globals_func.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"access__decl_8hpp.html",
"classhelios_1_1BasicCStringView.html#ac2718b8424052e8f467226fdcb549ed8",
"classhelios_1_1app_1_1App.html#ac3603ebfaf1c5a070f6024a1e8180e33",
"classhelios_1_1async_1_1Executor.html",
"classhelios_1_1async_1_1Task.html#ae776e77776ea246366fc4c988d355cfb",
"classhelios_1_1container_1_1BasicStaticString.html#ac2a05a9c4e3fa5707c7b3263cd9c054a",
"classhelios_1_1container_1_1MultiTypeMap.html#aa5b3701092540ab350f1b8003feb2b04",
"classhelios_1_1container_1_1TypedBufferArray.html#a277106ac80d000db5a21633108f778de",
"classhelios_1_1ecs_1_1Archetype.html#a3b5366e22d9a8fd4466ea9d3b7f7ddb7",
"classhelios_1_1ecs_1_1BasicMessageWriter.html#a051c666b9a617eccddd4ff27afd34998",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a639daf4e859712feba2b0ee1a2d80ac4",
"classhelios_1_1ecs_1_1ComponentManager.html#a40c02f693ff880d2d6fb67864703b962",
"classhelios_1_1ecs_1_1ConsumableMessageWrapperIter.html#a72d1848a65b2d48008b4ab4e0cc50475",
"classhelios_1_1ecs_1_1EntityCmdBuffer.html#ae1d1748be411a64d30b351c3b0d289be",
"classhelios_1_1ecs_1_1MessageQueue.html#a61d1955e914805535a6aaaef1cd4edd7",
"classhelios_1_1ecs_1_1MessageWrapperIter.html#aff843697d763c30a8f696ddbf9a888f0",
"classhelios_1_1ecs_1_1Scheduler.html#aa0d8e9a705819e722395c722d708902a",
"classhelios_1_1ecs_1_1SystemSetHandle.html#a476b56f27b1a5601751022c1d4c45a70",
"classhelios_1_1ecs_1_1World.html#ab23c40337d5a5330b31e73754402999e",
"classhelios_1_1mem_1_1AtomicRefCounted.html#a190395dd24f98367b5a7bc39bdfcfe35",
"classhelios_1_1mem_1_1RefCounted.html#ab7561a602056cc3cf2eccc8bcd52c0f7",
"classhelios_1_1profile_1_1ScopedZone.html#a7c60d112ce6ec29a48d2653ad3c076dc",
"classhelios_1_1utils_1_1FilterAdapter.html#a72d4d330718c70483b2d7e77ff8ad078",
"classhelios_1_1utils_1_1ReverseAdapter.html#a6d273277a5dcdf77a15ce7599db6b7d7",
"classhelios_1_1utils_1_1StrideAdapter.html#a28585cebaee0c18cd67bae8efad5d1de",
"classhelios_1_1utils_1_1ZipAdapter.html#a2a8dc24033773885c9fdf8bbf7cdb36a",
"cstring__view_8hpp.html#a0eae2881bdcf398546547f64adce687a",
"functions_func_u.html",
"md_docs_2guidelines.html#prerequisites",
"namespacehelios_1_1app.html#aaa2e7e163848df09000145bed33817e4",
"namespacehelios_1_1mem.html#ae38a06a39f091ee615bae93c111f6faa",
"profile_8hpp.html",
"structhelios_1_1app_1_1FixedRunnerConfig.html",
"structhelios_1_1ecs_1_1ResourceConflictInfo.html#a80d297812eb1328cff5ab3696b41a8b0",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01WorldView_01_4.html",
"structhelios_1_1profile_1_1ZoneSpec.html#a94e13218e1a4d6bd7a7c1027fe7cbcd3"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';