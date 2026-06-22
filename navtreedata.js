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
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#aa63e8061d6ff9395afc8928dab6cdbd5",
"classhelios_1_1app_1_1DynamicPlugin.html#ac2ec38e7678b63f9db8a0f389c058cff",
"classhelios_1_1async_1_1Executor.html#af936f62688db4c79bc88fe0c2c77b8d8",
"classhelios_1_1async_1_1TaskGraph.html#ae6f22b12a5832767569326c8130f8db7",
"classhelios_1_1container_1_1BasicStaticString.html#afbd0ebd3a64f238e6ac779ba33a6596e",
"classhelios_1_1container_1_1SparseSet.html#a344561306ae277b5dd9b0e9e34abf265",
"classhelios_1_1container_1_1TypedBufferArray.html#a6dcaaa813670781575b2760e607bc9d7",
"classhelios_1_1ecs_1_1ArchetypeId.html#a3ac7a3e26c8f92eb40d9528e5641fdd6",
"classhelios_1_1ecs_1_1BasicQuery.html#a5734f1913f3e5082895a9ef33843e40a",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#af3bf88f8c281d62775add80e9f28af1c",
"classhelios_1_1ecs_1_1ComponentManager.html#ae025639dc6677fdc6ff59477525799ac",
"classhelios_1_1ecs_1_1ConsumedMessagesRegistry.html#a8497912b4235355eed85b9a0622be4f4",
"classhelios_1_1ecs_1_1EntityManager.html#afe2382587c09acefb83d421b9b8a969b",
"classhelios_1_1ecs_1_1MessageReader.html#a5fc18d2d8c4f600f4e76280f297826a6",
"classhelios_1_1ecs_1_1ResourceManager.html#a05fa9c9a15c4f68a41dd9e7a9db76bfc",
"classhelios_1_1ecs_1_1SparseComponentStorage.html#a1df787b2fd75eb3d7f0fab7531928c69",
"classhelios_1_1ecs_1_1TryDestroyEntityCmd.html#aa90271182dfe4dc53fcbaaf352848262",
"classhelios_1_1ecs_1_1WorldCmdBuffer.html#a05f2b74a3480bede521383e997352633",
"classhelios_1_1mem_1_1FreeListAllocator.html#a20b9e7a6237969290e6d59a93fec0a98",
"classhelios_1_1profile_1_1Backend.html#a2f0d93e6897027ccb211ebfc91f80afe",
"classhelios_1_1utils_1_1ChainAdapter.html#a721626a9ee24136fac8453764cf94699",
"classhelios_1_1utils_1_1FunctionalAdapterBase.html#a7bddc9da14c1742058e0f0c816aa44ce",
"classhelios_1_1utils_1_1SkipWhileAdapter.html#a06a7bafb13a0156164401cae005c4277",
"classhelios_1_1utils_1_1TakeAdapter.html#a2def5bf84c73cb4fe6ff910ef393f20f",
"concepthelios_1_1app_1_1SubAppTrait.html",
"dir_efe3d36c8127beaa22cda24a5437cbee.html",
"macros_8hpp.html#ae3af7d8ee46470b8acc59dbc63207d73",
"namespacehelios_1_1ecs.html#a8e814f2d6b0d57fcb71731fc95c49e90a0edd03ba1f5a73f26d1fd764255ddaa1",
"namespacehelios_1_1utils.html#aca74c060b4e32cb671a0bc6fb378365eaba8ea63548a12106e8f5aed4e964ae6a",
"structhelios_1_1app_1_1MainStartup.html",
"structhelios_1_1ecs_1_1ResourceConflictInfo.html#a381a9506d7235a2769cfa4a6599c14ce",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01WorldView_01_4.html",
"structhelios_1_1profile_1_1ZoneSpec.html#a94e13218e1a4d6bd7a7c1027fe7cbcd3"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';