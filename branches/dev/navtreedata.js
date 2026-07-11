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
"classhelios_1_1app_1_1DynamicPlugin.html#a82476155e914f1cb74a168e93ca320c8",
"classhelios_1_1async_1_1Executor.html#a184b27053d1e6d5616d3bc6f0bbf391d",
"classhelios_1_1async_1_1Task.html#af709279b2a0ceeaedd7f7447ca140547",
"classhelios_1_1container_1_1BasicStaticString.html#ac8accafac1615d8a609e6c558a0d454b",
"classhelios_1_1container_1_1MultiTypeMap.html#ad11a026997ac5bc6ba87194205ad6dfb",
"classhelios_1_1container_1_1TypedBufferArray.html#a2a97c0aea0b2e12a7eafab4f16fa744a",
"classhelios_1_1ecs_1_1Archetype.html#a5613a69c2d95ae923973449f64781d09",
"classhelios_1_1ecs_1_1BasicMessageWriter.html#a919d296d5a84a4dc326284f0cba43f65",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a7543516975e568bce9bb8f6b6ea3953e",
"classhelios_1_1ecs_1_1ComponentManager.html#a59a0424b1b843b6755f3941b785ace99",
"classhelios_1_1ecs_1_1ConsumableMessageWrapperIter.html#aaf82d58665d06a904f81f90031032e77",
"classhelios_1_1ecs_1_1EntityDestroyedMsg.html#a2427a0e2e49030bdc7033c4972cd4e45",
"classhelios_1_1ecs_1_1MessageQueue.html#a965aafe62c111ea7d49e0141b7d5381d",
"classhelios_1_1ecs_1_1MultiThreadedExecutor.html#aa406b841b28eb0e36d2f21ddd1974054",
"classhelios_1_1ecs_1_1Scheduler.html#aac0b4a3d1811b4566e670418deb7a67e",
"classhelios_1_1ecs_1_1SystemSet.html#a7dcc941785c063aa59471f183709f4fd",
"classhelios_1_1ecs_1_1World.html#a51fb2d478a225f332448f3b1b8fa1c25",
"classhelios_1_1mem_1_1ArcFromThis.html#a59707b2a747fc8bdbe852254de6db55e",
"classhelios_1_1mem_1_1FrameAllocator.html#a4172d346fa6834a3cd9495da1fad5cab",
"classhelios_1_1profile_1_1Backend.html#a469d20934df8274fcf0e7c46581feecc",
"classhelios_1_1utils_1_1ChainAdapter.html#a8e2b8edb8500a1b0c3514185ff6463ec",
"classhelios_1_1utils_1_1FunctionalAdapterBase.html#aa9bda3e17a34788d84832d3ff13c0de1",
"classhelios_1_1utils_1_1SkipWhileAdapter.html#a3e5b997debb201547e5d72f5ffce05ca",
"classhelios_1_1utils_1_1TakeAdapter.html#a7d8245219a622977cf77fac7923e3f2e",
"concepthelios_1_1async_1_1AnyTask.html",
"dir_f3b2a97ca346c6259d024c9b6dcab06f.html",
"logger_8hpp.html",
"namespacehelios_1_1ecs.html#a2222a8ceaa7f9005447fc4e1bf219cb5",
"namespacehelios_1_1utils.html#a771b2844822c26c218b3f5692e3fb03b",
"structhelios_1_1app_1_1AppExit.html#adab7e429521146c717b54969a64e1f74",
"structhelios_1_1ecs_1_1ComponentManager_1_1SparseStorageEntry.html#a0e777d2d76831b01eb3195b04fde1af3",
"structhelios_1_1ecs_1_1SystemLocalData.html#a5e307d56e7e34074450f79d27a077968",
"structhelios_1_1ecs_1_1details_1_1MemberFnArgs_3_01R_07_5_08_07Args_8_8_8_08_4.html#adab526dddc88b6a495b1cf4cf4dc29f5",
"structstd_1_1hash_3_01helios_1_1container_1_1BasicStaticString_3_01StrCapacity_00_01CharT_00_01Traits_01_4_01_4.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';