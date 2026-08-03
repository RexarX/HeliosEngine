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
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#a7eb677265ad023c56dd50192f8dbca15",
"classhelios_1_1app_1_1DynamicPlugin.html#a62fa0c7bcc38be9bcde87e59f6cc5600",
"classhelios_1_1async_1_1Executor.html",
"classhelios_1_1async_1_1Task.html#ae7f412fe8a43d41545044d5aee299d45",
"classhelios_1_1container_1_1BasicStaticString.html#ac57647257a36aa11564699a04ca57d75",
"classhelios_1_1container_1_1MultiTypeMap.html#ab9a195e68d4eac0fd1fe16da6253389c",
"classhelios_1_1container_1_1TypedBufferArray.html#a27b294408c4abdb063881a72ba9d4e90",
"classhelios_1_1ecs_1_1Archetype.html#a20b61fce838de60dd024aec6649c830e",
"classhelios_1_1ecs_1_1AsyncMessageWriter.html#aba5b55c82cc27822e8313d163dec148a",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a4be538aefa7f8391d3138201aa52b20f",
"classhelios_1_1ecs_1_1ComponentBundle.html#ad04d3051ff280dd75b093ea706dac7fc",
"classhelios_1_1ecs_1_1ConsumableMessageWrapper.html#a56025f46fe4de7016b6078b6a295a077",
"classhelios_1_1ecs_1_1EntityAddedMsg.html#aa32d0b4ad61261b9bbec8b5b0c3dd8d5",
"classhelios_1_1ecs_1_1MessageManager.html#aae786195663d6f17a19e2aa84bbb5f59",
"classhelios_1_1ecs_1_1MessageWrapper.html#aea41db614ae5fa7507deed566e527e0e",
"classhelios_1_1ecs_1_1ScheduleOrdering.html#a1815bb36fe74135795ccc2fef2fae459",
"classhelios_1_1ecs_1_1SystemGroupHandle.html#af4bc3232d5bfbb735a1bbb5784cb4567",
"classhelios_1_1ecs_1_1TryRemoveBundleCmd.html#a158bf74189edc4397b911ba881f310a3",
"classhelios_1_1ecs_1_1WorldCmdBuffer.html#a2b79edce942f888d4a1b19f4ef960e0b",
"classhelios_1_1mem_1_1FixedArenaAllocator.html#a6b280c9c76c69b46ea01d2302d786941",
"classhelios_1_1mem_1_1RefCounted.html",
"classhelios_1_1profile_1_1Profiler.html#a79b4385bcc92f1551bee6645d449e4b2",
"classhelios_1_1utils_1_1FastPimpl.html",
"classhelios_1_1utils_1_1RandomGenerator.html#a151ce26c008dc61f122ebfcdd77f97b4",
"classhelios_1_1utils_1_1SlideView.html#af0f7d210bb91d0cf9b6ea63c36557913",
"classhelios_1_1utils_1_1TypeId.html#ab5c6708c75df8707c13fb793f7a6373b",
"concepthelios_1_1utils_1_1PredicateFor.html",
"functions_type_i.html",
"namespacehelios.html#a5e276a5c70bbb86023543f06d4c58d28",
"namespacehelios_1_1log.html#a719d5c971dea79590f40b51b05f9ab0d",
"namespacemembers_w.html",
"structhelios_1_1app_1_1ShutdownStage.html#a1f94c764a3ee14e687450508e9594016",
"structhelios_1_1ecs_1_1ResourceConflictInfo.html#a381a9506d7235a2769cfa4a6599c14ce",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01Res_3_01const_01T_01_4_01_4.html#af32dcf1613d1a251d34f8729021054a0",
"structhelios_1_1log_1_1DefaultLogger.html#a571a9c81ab62eff30f6652b2f7a9c6c6",
"timer_8hpp.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';