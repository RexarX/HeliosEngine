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
"classhelios_1_1async_1_1Executor.html#a0b25ee6fa34cea7bd0a152a06f4a83a6",
"classhelios_1_1async_1_1Task.html#ae9345588c4199e1c37216585178a6e16",
"classhelios_1_1container_1_1BasicStaticString.html#ac739527630641525cb415c1f4bdef3a2",
"classhelios_1_1container_1_1MultiTypeMap.html#abf4bb59aa5fab1b165d5f097a8ce8e5e",
"classhelios_1_1container_1_1TypedBufferArray.html#a28278a36802633102f8bbdf429851a20",
"classhelios_1_1ecs_1_1Archetype.html#a255640343b3c24d611f164a5397350dc",
"classhelios_1_1ecs_1_1AsyncMessageWriter.html#abbc561d206d99f51fd36085b19bbe665",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#a4d48d7d03e1b493d70c01a30dda3f7b5",
"classhelios_1_1ecs_1_1ComponentManager.html",
"classhelios_1_1ecs_1_1ConsumableMessageWrapper.html#a694879536d6fdc048c8bd65e793ea2f4",
"classhelios_1_1ecs_1_1EntityAddedMsg.html#abc3b849a08a8b04b723ca62990249ef1",
"classhelios_1_1ecs_1_1MessageManager.html#ab65d2ee1328e58e2ed1ca4dbe3fd5f3a",
"classhelios_1_1ecs_1_1MessageWrapperIter.html",
"classhelios_1_1ecs_1_1ScheduleOrdering.html#a2141bd649ad6c3f79f290283ec5ad67c",
"classhelios_1_1ecs_1_1SystemGroupHandle.html#af6c92366e4bddde84893da63d03dd7dd",
"classhelios_1_1ecs_1_1TryRemoveBundleCmd.html#a25fd9f9970395a3915510069aaf55917",
"classhelios_1_1ecs_1_1WorldCmdBuffer.html#a32914b1c86e2e0530241bf2da00f127f",
"classhelios_1_1mem_1_1FixedArenaAllocator.html#a87a06fbaf17b29fe300a28309dc8d25b",
"classhelios_1_1mem_1_1RefCounted.html#a11aa2c5715ce0d81501e47cfeae65ad7",
"classhelios_1_1profile_1_1Profiler.html#a7f914ea6f8623fd17ce5a28d35d3a6e0",
"classhelios_1_1utils_1_1FastPimpl.html#a14c88b000e29f1799c3de1f91dd3966b",
"classhelios_1_1utils_1_1RandomGenerator.html#a1fedf936562cadc8af6902fc731df29e",
"classhelios_1_1utils_1_1StepByAdapter.html",
"classhelios_1_1utils_1_1TypeId.html#adfa60d6504adaa0ba35d8920338216c2",
"concepthelios_1_1utils_1_1RandomEngine.html",
"functions_type_o.html",
"namespacehelios.html#a972743e2f2714ec185e080de5028f319",
"namespacehelios_1_1log.html#a8142b0365811db503db54adfe84e1837",
"namespaces.html",
"structhelios_1_1app_1_1Startup.html",
"structhelios_1_1ecs_1_1ResourceConflictInfo.html#a49f37ae292ed9e10903df3b4e0e9aa29",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01WorldView_01_4.html",
"structhelios_1_1log_1_1DefaultLogger.html#ad0bfeca11d53daed7a08ea3720a07b4c",
"tracy_8cpp.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';