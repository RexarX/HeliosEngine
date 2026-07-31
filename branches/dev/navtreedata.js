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
        [ "Building", "index.html#building", [
          [ "Linux (GCC)", "index.html#linux-gcc", null ],
          [ "Linux (Clang)", "index.html#linux-clang", null ],
          [ "Windows", "index.html#windows", null ],
          [ "macOS (Clang)", "index.html#macos-clang", null ],
          [ "Recommended developer flags", "index.html#recommended-developer-flags", null ]
        ] ],
        [ "Run the Example", "index.html#run-the-example", null ]
      ] ],
      [ "Usage", "index.html#usage-2", null ],
      [ "Architecture", "index.html#architecture", null ],
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
        [ "Registration (<span class=\"tt\">Module.cmake</span>)", "md_docs_2guidelines.html#registration-modulecmake", null ],
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
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"access__decl_8hpp.html",
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#a7ffd51bc5d6de517d4377b4e3cb856a2",
"classhelios_1_1app_1_1DynamicPlugin.html#a7517cbb034358deda18c4c73ac9538b5",
"classhelios_1_1async_1_1Executor.html#a104864e488e3e7893e61ad1efc8ce3f7",
"classhelios_1_1async_1_1Task.html#af38cc48187463e54736c41433274f690",
"classhelios_1_1container_1_1BasicStaticString.html#ac7b443443311dbe30d8b74be96a6167d",
"classhelios_1_1container_1_1MultiTypeMap.html#acccc5d27dcabf4c4530915d116a6fc30",
"classhelios_1_1container_1_1TypedBufferArray.html#a2a14c83b39ca2854fdbe1f96fba00009",
"classhelios_1_1ecs_1_1Archetype.html#a28fbe183c858de4a2bec96c7970a7cee",
"classhelios_1_1ecs_1_1AsyncMessageWriter.html#ad38c5244e2a034ee542fec7714e3f4d4",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a535cc9fc5747cf7656f853db91c3e264",
"classhelios_1_1ecs_1_1ComponentManager.html#a142153478594b8c40e51e7e4e80086e8",
"classhelios_1_1ecs_1_1ConsumableMessageWrapper.html#a93c86d3c678ff1258380036168fda0a2",
"classhelios_1_1ecs_1_1EntityCmdBuffer.html#a4ec6f5085ea24ec3337492e451448be2",
"classhelios_1_1ecs_1_1MessageManager.html#ad1e4814ee44e8de30e300a199de11566",
"classhelios_1_1ecs_1_1MessageWrapperIter.html#a4c756ed43d22298e814ea013d44b4c00",
"classhelios_1_1ecs_1_1ScheduleOrdering.html#a642ef4ceb484179bb2ba8431f41f9180",
"classhelios_1_1ecs_1_1SystemHandle.html#a0f94d034fd1f8e5b14bd47fed635ebd6",
"classhelios_1_1ecs_1_1TryRemoveComponentsCmd.html#a0bfb460ee29c4d2b4ec78a0d83943689",
"classhelios_1_1ecs_1_1WorldCmdBuffer.html#a75fc8268a3ab5b8ab947e06d7a36fa89",
"classhelios_1_1mem_1_1FixedFreeListAllocator.html",
"classhelios_1_1mem_1_1RefCounted.html#a5364bca3ebba074f3374fc0a2a46c188",
"classhelios_1_1profile_1_1Profiler.html#aa03d6a16e8e81b14e7aebf8b8d71ddf4",
"classhelios_1_1utils_1_1FastPimpl.html#aac49a356ec97da30fdf346a9766844e0",
"classhelios_1_1utils_1_1RandomGenerator.html#ab0db0436e2d6ee8b47a0bc257b87e18f",
"classhelios_1_1utils_1_1StepByAdapter.html#a280022b7a9f029976141cc621e631e09",
"classhelios_1_1utils_1_1TypeIndex.html#a3cecf19d1c9867e18b73c1b631a4d0c7",
"concepthelios_1_1utils_1_1StrideAdapterRequirements.html",
"functions_vars_d.html",
"namespacehelios_1_1app.html#a204a57a85ac876ab0950fcd4ecce94c4",
"namespacehelios_1_1log.html#af1a1fc368da4e12ebbbcd0ba3d5bc856",
"plugin_8hpp.html",
"structhelios_1_1app_1_1Time.html#a25a0f9a6ab2e2548c3a345b6656bf527",
"structhelios_1_1ecs_1_1ResourceInsertedMsg.html#ad28eeec6ce112a5be5e9a6727fd0d045",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01std_1_1optional_3_01Res_3_01const_01T_01_4_01_4_01_4.html#af9ff4d318e9ebc47e0563dcebc354168",
"structhelios_1_1mem_1_1ArenaOptions.html",
"uuid_8hpp_source.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';