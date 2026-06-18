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
      [ "Building", "md_docs_2guidelines.html#building", [
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
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#aa92cb4e9496f7eebd58f68de0a652563",
"classhelios_1_1app_1_1DynamicPlugin.html#acdb4ff84ee5ba0c2f0be23043a8a5d47",
"classhelios_1_1async_1_1Executor.html#afa2cdb7c2fdea0737800ebbe28847203",
"classhelios_1_1async_1_1TaskGraph.html#ae9345588c4199e1c37216585178a6e16",
"classhelios_1_1container_1_1BasicStaticString.html#afc8b8f2bc646d8ad31d84380c17ae8fa",
"classhelios_1_1container_1_1SparseSet.html#a3889d6f69f2ce7fb7532fbf40c0d9047",
"classhelios_1_1container_1_1TypedBufferArray.html#a6dcaaa813670781575b2760e607bc9d7",
"classhelios_1_1ecs_1_1ArchetypeId.html#a34a4bfbda60b2fd2f8ddc6a958ee90e0",
"classhelios_1_1ecs_1_1BasicQuery.html#a5013090f7e73748ad78e9762c32549fe",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#aec951e525c74dfef7f0d2fbae946eca3",
"classhelios_1_1ecs_1_1ComponentManager.html#ad7f50acbfc2173d765da2ff3f7213078",
"classhelios_1_1ecs_1_1ConsumedMessagesRegistry.html#a52baae77b079a420fa27be835cbd7ec1",
"classhelios_1_1ecs_1_1EntityManager.html#aa47bd00fe78174b9f38ba865481d9821",
"classhelios_1_1ecs_1_1MessageReader.html#a328e776a75f65f56c59876345900994c",
"classhelios_1_1ecs_1_1Res.html#ad4fda7cdf66668d6d2796b48fb06b64d",
"classhelios_1_1ecs_1_1SparseComponentStorage.html",
"classhelios_1_1ecs_1_1TryDestroyEntitiesCmd.html#a9dc1b4c7a3828f530790febe39687e14",
"classhelios_1_1ecs_1_1World.html#afb4b8db5d665eabe721d8cba1207ee77",
"classhelios_1_1mem_1_1AtomicRefCounted.html#ad81740fe59564e41c296e711149173d7",
"classhelios_1_1profile_1_1Backend.html#a000645aa569d0216ddede3a12abe5c60",
"classhelios_1_1utils_1_1ChainAdapter.html#a24d079ad5a4b289c447a207a9d7e5f9f",
"classhelios_1_1utils_1_1FunctionalAdapterBase.html#a6a31b42112f20291cb0a04d8e93a8f00",
"classhelios_1_1utils_1_1SkipAdapter.html#aef24e8db0f0be49f476b5f41eb13ce3f",
"classhelios_1_1utils_1_1TakeAdapter.html#a10bdc858f5fbed1bfdc6155e2ed5717d",
"composite__param_8hpp.html",
"dir_d28a4824dc47e487b107a5db32ef43c4.html",
"macros_8hpp.html#adc8086efeaa01584ac9bf9218414005b",
"namespacehelios_1_1ecs.html#a7dd9b5d716679ce5ef8671f7f11aba2b",
"namespacehelios_1_1utils.html#aca74c060b4e32cb671a0bc6fb378365ea5111e24c1ecc6266ce0de4b4dc42033b",
"structhelios_1_1app_1_1MainStartup.html#acdce21f84721365fc5b18812a1f8fd37",
"structhelios_1_1ecs_1_1ResourceConflictInfo.html#a49f37ae292ed9e10903df3b4e0e9aa29",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01Res_3_01const_01T_01_4_01_4.html#af32dcf1613d1a251d34f8729021054a0",
"structhelios_1_1profile_1_1ZoneSpec.html#a13fd67327bde6b653693c1f000a4adb7"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';