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
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#a967ac7ecaa2cc04d793f34b54b555e55",
"classhelios_1_1app_1_1DynamicPlugin.html#ab9135603191cf66bb39b194d54082b0c",
"classhelios_1_1async_1_1Executor.html#abce40b45e5a02705a72bf2a8620250aa",
"classhelios_1_1async_1_1TaskGraph.html#a763b2f90bc53f92d680a635fe28e858e",
"classhelios_1_1container_1_1BasicStaticString.html#aedefbc44d361bf2c8396812d0ae3f5ac",
"classhelios_1_1container_1_1SparseSet.html#a05904744bd2cd54af22795ddc0d5339d",
"classhelios_1_1container_1_1TypedBufferArray.html#a4b2e01f5b5194cea58e6c7b78b58195b",
"classhelios_1_1ecs_1_1Archetype.html#af8e5a25765582cb89ed3e17a8bfaae9e",
"classhelios_1_1ecs_1_1BasicQuery.html#a42823578847b69bc3089ce53b3abdb22",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#ad04f9b9f226f85322dad11a417d9447b",
"classhelios_1_1ecs_1_1ComponentManager.html#aa52200da1ac7b99bbdb29f8cc9f0fbe0",
"classhelios_1_1ecs_1_1ConsumedMessagesRegistry.html#a2135114e6759c30d858e971f8fba216d",
"classhelios_1_1ecs_1_1EntityManager.html#a42e43f9934ceeddc5e1aad3a34bf4d12",
"classhelios_1_1ecs_1_1MessageQueue.html#aef5cbb87977aa8573c34ed7c6008a1f8",
"classhelios_1_1ecs_1_1Res.html#a596c3ed86036e8ab0fa2434752dd549f",
"classhelios_1_1ecs_1_1SingleThreadedExecutor.html#a5bc17144ab951100d9b9b3c0e781ffcf",
"classhelios_1_1ecs_1_1SystemSetHandle.html#a03266c7fb6c8490f865dd683d5abf8a0",
"classhelios_1_1ecs_1_1World.html#a7f1f83bc480c7f0e050b124f8e9c01fa",
"classhelios_1_1mem_1_1ArenaAllocator.html#a41985abb41075e2374393bab6beaa9a2",
"classhelios_1_1mem_1_1FreeListAllocator.html",
"classhelios_1_1profile_1_1FlamegraphBackend.html#a1b59ee06e939d99b3c06b49ae53725cd",
"classhelios_1_1utils_1_1Defer.html#a94423f3b7b72c05f1a1625f9db81ff8f",
"classhelios_1_1utils_1_1InspectAdapter.html#a1f6dc03dd85cd0632b9787ba72c2e69e",
"classhelios_1_1utils_1_1SkipWhileAdapter.html#ad72117049eb13a741ff009deebf677bc",
"classhelios_1_1utils_1_1TakeWhileAdapter.html#a516a1779b05742927fa3071a7dad9a21",
"concepthelios_1_1ecs_1_1MessageWithNameTrait.html",
"files.html",
"macros_8hpp.html#abeef0bde870eaf7b42d8c467e2c7dd40",
"namespacehelios_1_1ecs.html#a7dd9b5d716679ce5ef8671f7f11aba2b",
"namespacehelios_1_1utils.html#aca74c060b4e32cb671a0bc6fb378365eac11f7f7d37763d1430ed2682642ea0ac",
"structhelios_1_1app_1_1Extract.html#a6001dee9752a7d9d867303efe15b7f39",
"structhelios_1_1ecs_1_1ComponentManager_1_1SparseStorageEntry.html#ab7ee259563151b84f348034b82b8107d",
"structhelios_1_1ecs_1_1SystemParamTraits.html",
"structhelios_1_1log_1_1Config.html#a597150506bbb30d3d469257b1cabd7d2",
"structstd_1_1hash_3_01helios_1_1utils_1_1TypeId_01_4.html#a933062527be8e4a671ff410495594564"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';