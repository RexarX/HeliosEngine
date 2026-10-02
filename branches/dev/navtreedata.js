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
          [ "All platforms &ndash; build tools", "index.html#all-platforms----build-tools", null ],
          [ "Linux (APT &ndash; Ubuntu / Debian)", "index.html#linux-apt----ubuntu--debian", null ],
          [ "Linux (DNF &ndash; Fedora)", "index.html#linux-dnf----fedora", null ],
          [ "Linux (Pacman &ndash; Arch)", "index.html#linux-pacman----arch", null ],
          [ "macOS (Homebrew)", "index.html#macos-homebrew", null ],
          [ "Windows (MSVC)", "index.html#windows-msvc", null ]
        ] ],
        [ "Building", "index.html#building", [
          [ "Linux (GCC)", "index.html#linux-gcc", null ],
          [ "Linux (Clang)", "index.html#linux-clang", null ],
          [ "Windows", "index.html#windows", null ],
          [ "macOS (Clang)", "index.html#macos-clang", null ],
          [ "Recommended developer flags", "index.html#recommended-developer-flags", null ]
        ] ],
        [ "C++20 modules", "index.html#c20-modules", null ],
        [ "Linking", "index.html#linking", null ],
        [ "Run the Example", "index.html#run-the-example", null ]
      ] ],
      [ "Usage", "index.html#usage-6", null ],
      [ "Using as a Dependency", "index.html#using-as-a-dependency", [
        [ "Method 1: <span class=\"tt\">add_subdirectory</span>", "index.html#method-1-add_subdirectory", null ],
        [ "Method 2: <span class=\"tt\">FetchContent</span>", "index.html#method-2-fetchcontent", null ],
        [ "Method 3: CPM", "index.html#method-3-cpm", null ],
        [ "Method 4: Installed Package (<span class=\"tt\">find_package</span>)", "index.html#method-4-installed-package-find_package", null ]
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
        [ "Build definition (<span class=\"tt\">CMakeLists.txt</span>)", "md_docs_2guidelines.html#build-definition-cmakeliststxt", null ],
        [ "Selective module builds", "md_docs_2guidelines.html#selective-module-builds", null ]
      ] ],
      [ "Testing", "md_docs_2guidelines.html#testing", [
        [ "Structure", "md_docs_2guidelines.html#structure", null ],
        [ "Check macros", "md_docs_2guidelines.html#check-macros", null ],
        [ "Running tests", "md_docs_2guidelines.html#running-tests", null ]
      ] ],
      [ "Building", "md_docs_2guidelines.html#building-1", [
        [ "Prerequisites", "md_docs_2guidelines.html#prerequisites", null ],
        [ "CMake presets", "md_docs_2guidelines.html#cmake-presets", null ]
      ] ],
      [ "Developer scripts", "md_docs_2guidelines.html#developer-scripts", [
        [ "Formatting enforcement", "md_docs_2guidelines.html#formatting-enforcement", null ]
      ] ],
      [ "Further reading", "md_docs_2guidelines.html#further-reading", null ]
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
        [ "Variables", "functions_vars.html", "functions_vars" ],
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
        [ "Enumerator", "globals_eval.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"access__decl_8hpp.html",
"classhelios_1_1BasicCStringView.html#ade5991d83f0402255f24204f4858e094",
"classhelios_1_1app_1_1App.html#ab8033d1c1056a5b0b8469c616e9f6d14",
"classhelios_1_1app_1_1SubApp.html#a2000f9207358503e07775cba7c912dcc",
"classhelios_1_1async_1_1SubTaskGraph.html#a3b9eb5e36f4dc25e03d10aca0a67b313",
"classhelios_1_1container_1_1BasicStaticString.html#a391c8cd1cd19db9ec120a5b5f6b6afcf",
"classhelios_1_1container_1_1CallableBufferArray.html#a6f7e49f1ba6a40f1a2fcf528bcdde388",
"classhelios_1_1container_1_1MultiTypeMap.html#a4ee19d1f6c58f263458a4d23e156fc98",
"classhelios_1_1container_1_1TypedBuffer.html#a4929e6f34a14d72bff46ac987637290d",
"classhelios_1_1ecs_1_1AccessPolicy.html#ac7a8289a12a69129bf0b15f7d91faf36",
"classhelios_1_1ecs_1_1AsyncMessageQueue.html#ae23dd576f19a06f4b8a1cd8f1b0921b1",
"classhelios_1_1ecs_1_1BasicQuery.html#af9aad477b27deeb53651a623249de1df",
"classhelios_1_1ecs_1_1CmdQueue.html#a00724f31a07d4efd354a1fbf61b6e1de",
"classhelios_1_1ecs_1_1ComponentTypeInfo.html#aa71a9b9a79f80e6b1777722dda26a721",
"classhelios_1_1ecs_1_1DestroyEntitiesCmd.html#a27001791e073075b36bf60dd736e21f8",
"classhelios_1_1ecs_1_1InsertResourceCmd.html#a15b719e61a0e8a06f12fd48d851209ca",
"classhelios_1_1ecs_1_1MessageReader.html#a7bc66b88442135f0463d77e300413de6",
"classhelios_1_1ecs_1_1Res.html#a897238eb1ec2cd77e2a835323392f97e",
"classhelios_1_1ecs_1_1Scheduler.html#abdae3abe91a6a1707e56ab4257c6cda4",
"classhelios_1_1ecs_1_1SystemSet.html#a58578f6301e433b5a28512292a21ac25",
"classhelios_1_1ecs_1_1World.html",
"classhelios_1_1input_1_1Axis.html#aaeae3877f9d09a2163b19becd5817d30",
"classhelios_1_1mem_1_1FixedPoolAllocator.html",
"classhelios_1_1mem_1_1RefCounted.html#ade5728490ffb3ad7780bc5da05ffa284",
"classhelios_1_1profile_1_1Profiler.html#a8d776cd0adfb275a5cefdb91480b8682",
"classhelios_1_1utils_1_1DynamicLibrary.html#a28be664016bb89052b096f6307a0f5cd",
"classhelios_1_1utils_1_1InspectAdapter.html#a324f41a5c8b9bae3bb093f9e5fdc83e3",
"classhelios_1_1utils_1_1SlideAdapter.html#a0478bb893444cb1951054fad9ebcab00",
"classhelios_1_1utils_1_1TakeWhileAdapter.html#a836ca84d59c9078ce9b4fb770ea6b359",
"concepthelios_1_1ecs_1_1AnyMessageTrait.html",
"dir_3b32cc936c84a91d953c326eac20816a.html",
"functions_func_n.html",
"lock_8hpp.html#a4d1b1bee9069ea0de5fcb51218481fb3",
"namespacehelios_1_1container.html#a75851f3bcb3db11a178838920fa6cbc5",
"namespacehelios_1_1input.html#a0e4d05c9a0b658544c038aebdf1af8fa",
"namespacehelios_1_1input.html#a9428ae91737625870558a82a3612d997a2d5fde1d924910a2a01ecd8e70a87c28",
"namespacehelios_1_1log.html",
"namespacehelios_1_1sdl3_1_1window_1_1anonymous__namespace_02event__handlers_8cpp_03.html#a77b8e2355cdd8c91614d57dfef4c3531",
"namespacehelios_1_1window.html#a6cd5e363d43bc8c5a13f276574070c6c",
"run__scope_8hpp_source.html",
"structhelios_1_1app_1_1FixedRunnerConfig.html#a68b368bfd65418a0c4d8cc1f74d3d54e",
"structhelios_1_1details_1_1MemberFunctionTraits_3_01R_07C_1_1_5_08_07Args_8_8_8_08_4.html#aae7e934af6102c13ae0bfc4a8f44a222",
"structhelios_1_1ecs_1_1ResourceInsertedMsg.html#aa00b48142af4112421b9e948562f5308",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01Query_3_01Args_8_8_8_01_4_01_4.html",
"structhelios_1_1ecs_1_1details_1_1HasStructBundleBuild.html",
"structhelios_1_1glfw_1_1PollEvents.html#ab75cc5a0cbb5af145ec3b67560c12fca",
"structhelios_1_1input_1_1GamepadMappings.html#ad92b4fe393c8c8087c5af34c7d9665d3",
"structhelios_1_1input_1_1JoystickMessages.html#a675ab8189e2afab39d72a43cf9300a6a",
"structhelios_1_1input_1_1Pen.html#a9bca9fd52124092846060076f518216c",
"structhelios_1_1input_1_1Settings.html#af7daa2e8c81c9c6c79cd531b64a04db3",
"structhelios_1_1log_1_1Config.html#af2ba67ecf080a03bc15df1c01fe18eee",
"structhelios_1_1sdl3_1_1input_1_1Context.html",
"structhelios_1_1sdl3_1_1window_1_1Plugin.html#acda3301033a4c991a8f450b0ac7f47b5",
"structhelios_1_1window_1_1ClosedMsg.html#a2ec5fad0158c162480901620b0f65138",
"structhelios_1_1window_1_1Messages.html#a954fc9bf93b54a3ee9ee4fb9f17f1d41",
"structhelios_1_1window_1_1Properties.html#a91b08359377827162d951c4860b99f31",
"structstd_1_1formatter_3_01helios_1_1ecs_1_1EntityDestroyedMsg_01_4.html#af83079c9466ce7cd9fc4b86c1cdc60d3",
"structstd_1_1formatter_3_01helios_1_1input_1_1MouseMotionMsg_01_4.html#a40467033354ed6e25e84812a61e8e8ca",
"structstd_1_1formatter_3_01helios_1_1window_1_1ExclusiveVideoMode_01_4.html",
"touch_8cpp_source.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';