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
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a42652457554894d1ab57130518a9e504",
"classhelios_1_1ecs_1_1ComponentBundle.html#aad5fadf534e9b8f93f9a15500c856b27",
"classhelios_1_1ecs_1_1ConsumableMessageWrapper.html#a2da3785d2c6c02ec7ec623516e0035ca",
"classhelios_1_1ecs_1_1EntityAddedMsg.html#a809a8c8e1b4f34872cbccd77eb6f558c",
"classhelios_1_1ecs_1_1MessageManager.html#aa79af7be2f5376667c8363226d6a8943",
"classhelios_1_1ecs_1_1MessageWrapper.html#ad45100fe9e12990de048c738b461c2e6",
"classhelios_1_1ecs_1_1ScheduleOrdering.html",
"classhelios_1_1ecs_1_1SystemGroupHandle.html#ad57e9a908cda2999c405531525098a61",
"classhelios_1_1ecs_1_1TryRemoveBundleCmd.html",
"classhelios_1_1ecs_1_1WorldCmdBuffer.html#a22541d0a042eb2cf084b8534681017cb",
"classhelios_1_1mem_1_1FixedArenaAllocator.html#a4bae968faf948969ae18aa1ac9ef1b61",
"classhelios_1_1mem_1_1RcFromThis.html#ac20d1941493daba6aded88aa5598911f",
"classhelios_1_1profile_1_1Profiler.html#a7283a6e2eb6624b43334aa4edebe4f8c",
"classhelios_1_1utils_1_1DynamicLibrary.html#ae7b810d0eb51c4fd4fa6dd7c037f3ff1",
"classhelios_1_1utils_1_1MapAdapter.html",
"classhelios_1_1utils_1_1SlideAdapter.html#acfc43c5d15523dc1cec077eed49e7a93",
"classhelios_1_1utils_1_1Timer.html#a5745c34072a0808f6417f0ac4d44b1c9",
"concepthelios_1_1mem_1_1PmrAllocator.html",
"functions_d.html",
"memory_2include_2helios_2memory_2memory_8hpp.html",
"namespacehelios_1_1ecs.html#abd1fa5ad164fafc85b0908aac3fbf20ba03fd5b5c9577b8248bfb9a2f39f770e4",
"namespacehelios_1_1utils_1_1details.html#a8fe570bae888c6eabee42a6bb21639c0",
"structhelios_1_1app_1_1FrameCount.html#a8443af6460d140d4a1bf94511457bc07",
"structhelios_1_1ecs_1_1ComponentMetadata.html#a7f2485ffa22171783904ad8488938d96",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01Commands_01_4.html#acf32754ee20434b8fe6f2b534933dfd2",
"structhelios_1_1ecs_1_1details_1_1RegisterQueryAccess.html",
"structstd_1_1hash_3_01helios_1_1container_1_1BasicStaticString_3_01StrCapacity_00_01CharT_00_01Traits_01_4_01_4.html#a2c471af96dd788c02164c47f197de1ab"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';