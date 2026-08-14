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
      [ "Usage", "index.html#usage-3", null ],
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
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"access__decl_8hpp.html",
"classhelios_1_1Delegate_3_01ReturnType_07Args_8_8_8_08_4.html#a320fd9befa0ceb10d4fc55e77f9a3893",
"classhelios_1_1app_1_1DynamicPlugin.html#a19268979bef556fbc15243755b76de2d",
"classhelios_1_1app_1_1TimePlugin.html",
"classhelios_1_1async_1_1Task.html#a1acaccc55db875a1891c86b72310b0ec",
"classhelios_1_1container_1_1BasicStaticString.html#a9a67a70354dffabac7a632fb08afc17a",
"classhelios_1_1container_1_1MultiTypeMap.html#a2c677274466a27a337234e700b4431e3",
"classhelios_1_1container_1_1TypedBuffer.html#ab719d7fccd087a9bf5f1062c54a969d7",
"classhelios_1_1ecs_1_1AddBundleCmd.html",
"classhelios_1_1ecs_1_1AsyncMessageWrapper.html#a4049e1e478fbdb4d8d707bdd12005717",
"classhelios_1_1ecs_1_1BasicQueryIter.html#aecf64e282c63a92235c7014401cb469c",
"classhelios_1_1ecs_1_1Commands.html#aa4bde140a62b2e5978a61d7843e3e8dc",
"classhelios_1_1ecs_1_1ConsumableMessageReader.html#a84daea978ba5f20cef89c8dcffed68e6",
"classhelios_1_1ecs_1_1Entity.html#ab49036ecc9b866d817fd798dcde0b8e6",
"classhelios_1_1ecs_1_1ManagedMessageWriter.html#ad854e33bb51e1d3845cdacf16e7f15f3",
"classhelios_1_1ecs_1_1MessageReaderBase.html#a780a27355594562e56f545a50c73c68a",
"classhelios_1_1ecs_1_1Schedule.html#a221472b7e946bfae9b23c1c31085c7b5",
"classhelios_1_1ecs_1_1SparseComponentStorage.html#ad17f8de977f05c44f5a72178319b6fab",
"classhelios_1_1ecs_1_1SystemSetHandle.html#ad0961fd24efebb8d15b4e8f2767c401a",
"classhelios_1_1ecs_1_1World.html#a8d170ada7705c7412f5f7e42e8bde0c4",
"classhelios_1_1log_1_1Logger.html#af9746dd513ff2a278d414cdef78261b9",
"classhelios_1_1mem_1_1FrameAllocator.html#a1e2584b7c7ee4b832687cb26bcf9ac80",
"classhelios_1_1profile_1_1Backend.html#a38a105e82c51f2fab5b56c44876cd9aa",
"classhelios_1_1utils_1_1ChainAdapter.html#a7c857ec9265811bb09af41bb0b8884b5",
"classhelios_1_1utils_1_1FunctionalAdapterBase.html#a05500143c093bdd31d54f55d013177d4",
"classhelios_1_1utils_1_1SkipAdapter.html#a357763e824b3faaaff474254757cf1cc",
"classhelios_1_1utils_1_1StrideAdapter.html#aa2494dcd33804767eba18df73f721901",
"classhelios_1_1utils_1_1ZipAdapter.html#ac81c3e172977e8c60b72a85475fcb3cf",
"dir_02d4378e96a5748eac7147fd9b05eed8.html",
"gamepad_8hpp.html",
"namespacehelios.html#afc2e2d6e5d9076d42481c5837df0f6ba",
"namespacehelios_1_1glfw.html#a55a9b5a5410ae3ff92aa048c4ec1712f",
"namespacehelios_1_1input.html#afa204f0427b65dedb7581da7ee6f5130",
"namespacehelios_1_1window.html#a19cd60ad8b1491acaae60e2681833c07a4bea75bbc2df5d8efddd7cec6a043901",
"structhelios_1_1StacktraceConfig.html#a7f7ff216be82ba1215341ea34d29c5c2",
"structhelios_1_1details_1_1MemberFunctionTraits_3_01R_07C_1_1_5_08_07Args_8_8_8_08_4.html#aa5b8de48b529a8bbfd497caba38e721a",
"structhelios_1_1ecs_1_1ResourceRemovedMsg.html",
"structhelios_1_1ecs_1_1SystemParamTraits_3_01Query_3_01Args_8_8_8_01_4_01_4.html#aea54a750a59aedb7a4531e94eca80ec9",
"structhelios_1_1ecs_1_1details_1_1IsComponentBundle_3_01T_01_4.html",
"structhelios_1_1input_1_1GamepadConnectionMsg.html#a78ffdf64869371bf9693f0b97b96576e",
"structhelios_1_1input_1_1Writers.html#a9444c1774a1fb6ac90c85b0c28d38a3c",
"structhelios_1_1window_1_1AppearanceWriters.html#a51f3750d695b7e5d341c82ab7631ce22",
"structhelios_1_1window_1_1HoverChangedMsg.html#ae04197b4320f8fec77e7a193dee85ba2",
"structhelios_1_1window_1_1Properties.html#a32837aaee99ebf046e998f356207886b",
"structstd_1_1formatter_3_01helios_1_1input_1_1GamepadButton_01_4.html"
];

const SYNCONMSG = 'click to disable panel synchronization';
const SYNCOFFMSG = 'click to enable panel synchronization';
const LISTOFALLMEMBERS = 'List of all members';