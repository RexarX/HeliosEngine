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
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#aa021385d636b7d83de50d31296ab1ed3",
"classhelios_1_1app_1_1DynamicPlugin.html#abbb1ba75dc8609dc3b98bb39cb627272",
"classhelios_1_1async_1_1Executor.html#aeb73a0112ec5245f84b03da8a97da43f",
"classhelios_1_1async_1_1TaskGraph.html#ae627df1fb6a154985c0ed317ea1efbba",
"classhelios_1_1container_1_1BasicStaticString.html#afa251c37825285a03ce11d2241ffff4f",
"classhelios_1_1container_1_1SparseSet.html#a319b6e1db875126cdc0452282ac1c384",
"classhelios_1_1container_1_1TypedBufferArray.html#a6d8159ac1982080562819fb8ad46d9e8",
"classhelios_1_1ecs_1_1ArchetypeId.html#a3a7aa7dd4128849b2700fca544dd8ded",
"classhelios_1_1ecs_1_1BasicQuery.html#a556a33a5c1ebc7243b92b059a888a91a",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#af3886f078114bf08c3a7bdd088d4e383",
"classhelios_1_1ecs_1_1ComponentManager.html#adb42cd3698f109e371da2c70405794fa",
"classhelios_1_1ecs_1_1ConsumedMessagesRegistry.html#a6723cd2ff3fed61873b1a8cce38e94c8",
"classhelios_1_1ecs_1_1EntityManager.html#ae142132888fb9323c171542864b4d104",
"classhelios_1_1ecs_1_1MessageReader.html#a47816284a8c2568ae46e36d93aa4fa6b",
"classhelios_1_1ecs_1_1ResourceManager.html#a026a1b0a3bd768227642be5072715450",
"classhelios_1_1ecs_1_1SparseComponentStorage.html#a13b26ff0cabc73970084a46602e9a494",
"classhelios_1_1ecs_1_1SystemSetHandle.html#a471fce9f0807ec1b137856c1900b8d5e",
"classhelios_1_1ecs_1_1World.html#aade6253489b550ad22f05557cda56a6f",
"classhelios_1_1mem_1_1AtomicRefCounted.html#a17f35fc0c4de5fc3056bf0bf6f80ac20",
"classhelios_1_1mem_1_1FreeListAllocator.html#a85c393075c98449a150784e77521d5c8",
"classhelios_1_1profile_1_1FlamegraphBackend.html#a40aa023f57014f1e84fd2fa1a1573afb",
"classhelios_1_1utils_1_1DynamicLibrary.html#a3ac68198100c24ccfe78f46864b1c7ff",
"classhelios_1_1utils_1_1InspectAdapter.html#a68ecfdb71f244f33b23dfb9bc9f67068",
"classhelios_1_1utils_1_1SlideAdapter.html#a2246d8d7e5deaf1fd5138e95fdc66f3b",
"classhelios_1_1utils_1_1TakeWhileAdapter.html#aa3006d36e959350cc4434b4d8cfe1f8e",
"concepthelios_1_1ecs_1_1StageTrait.html",
"flamegraph_8hpp.html",
"md_docs_2guidelines.html#module-structure",
"namespacehelios_1_1ecs.html#aa156d33741937b2c039927f9c831105a",
"namespacehelios_1_1utils_1_1details.html#aed24e26b1df2da90fda7bd6d3fe47218",
"structhelios_1_1app_1_1PreShutdown.html",
"structhelios_1_1ecs_1_1ResourceRemovedMsg.html#ac315af71c3c5105480808b789eade655",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01std_1_1optional_3_01Res_3_01const_01T_01_4_01_4_01_4.html#aee602c69d0e05603078478c99816921a",
"structhelios_1_1utils_1_1StringEqual.html#aa682f54eb3f2bda220f1bb6a18e07ef5"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';