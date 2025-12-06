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
    [ "README", "md_src_2core_2README.html", [
      [ "Table of Contents", "index.html#autotoc_md45", null ],
      [ "About The Project", "index.html#autotoc_md47", [
        [ "Key Features", "index.html#autotoc_md48", null ],
        [ "Design Philosophy", "index.html#autotoc_md49", null ]
      ] ],
      [ "Architecture", "index.html#autotoc_md51", [
        [ "Entity Component System (ECS)", "index.html#autotoc_md52", [
          [ "<strong>Entities</strong>", "index.html#autotoc_md53", null ],
          [ "<strong>Components</strong>", "index.html#autotoc_md54", null ],
          [ "<strong>Systems</strong>", "index.html#autotoc_md55", null ],
          [ "<strong>World</strong>", "index.html#autotoc_md56", null ],
          [ "<strong>Scheduling</strong>", "index.html#autotoc_md57", null ]
        ] ],
        [ "Modular Design", "index.html#autotoc_md58", null ]
      ] ],
      [ "Getting Started", "index.html#autotoc_md60", [
        [ "Requirements", "index.html#autotoc_md61", null ],
        [ "Dependencies", "index.html#autotoc_md62", [
          [ "Core Dependencies", "index.html#autotoc_md63", null ],
          [ "Test Dependencies", "index.html#autotoc_md64", null ],
          [ "Installation Methods", "index.html#autotoc_md65", null ]
        ] ],
        [ "Quick Start", "index.html#autotoc_md66", [
          [ "1. Clone the Repository", "index.html#autotoc_md67", null ],
          [ "2. Install Dependencies", "index.html#autotoc_md68", null ],
          [ "3. Configure the Project", "index.html#autotoc_md69", null ],
          [ "4. Build the Project", "index.html#autotoc_md70", null ],
          [ "5. Run Tests", "index.html#autotoc_md71", null ]
        ] ],
        [ "Build Options", "index.html#autotoc_md72", [
          [ "Using Makefile (Recommended)", "index.html#autotoc_md73", null ],
          [ "Using Python Scripts Directly", "index.html#autotoc_md74", null ],
          [ "Using CMake Presets", "index.html#autotoc_md75", null ]
        ] ]
      ] ],
      [ "Usage Examples", "index.html#autotoc_md77", [
        [ "Basic Application Setup", "index.html#autotoc_md78", null ],
        [ "Creating Entities", "index.html#autotoc_md79", null ],
        [ "Querying Entities", "index.html#autotoc_md80", null ],
        [ "Using Modules", "index.html#autotoc_md81", null ]
      ] ],
      [ "Core Module", "index.html#autotoc_md83", [
        [ "Overview", "index.html#autotoc_md84", null ]
      ] ],
      [ "Roadmap", "index.html#autotoc_md86", [
        [ "Completed", "index.html#autotoc_md87", null ],
        [ "In Progress", "index.html#autotoc_md88", [
          [ "<strong>Runtime modules</strong>", "index.html#autotoc_md89", null ],
          [ "<strong>Renderer modules</strong>", "index.html#autotoc_md90", null ]
        ] ]
      ] ],
      [ "Acknowledgments", "index.html#autotoc_md92", null ],
      [ "License", "index.html#autotoc_md94", null ],
      [ "Contact", "index.html#autotoc_md96", null ],
      [ "Helios Engine - Core Module", "md_src_2core_2README.html#autotoc_md0", [
        [ "Table of Contents", "md_src_2core_2README.html#autotoc_md2", null ],
        [ "Overview", "md_src_2core_2README.html#autotoc_md4", null ],
        [ "Architecture", "md_src_2core_2README.html#autotoc_md6", [
          [ "Scheduling", "md_src_2core_2README.html#autotoc_md7", null ],
          [ "Entity Component System (ECS)", "md_src_2core_2README.html#autotoc_md8", [
            [ "Entities", "md_src_2core_2README.html#autotoc_md9", null ],
            [ "Components", "md_src_2core_2README.html#autotoc_md10", null ],
            [ "Systems", "md_src_2core_2README.html#autotoc_md11", null ],
            [ "Events", "md_src_2core_2README.html#autotoc_md12", null ],
            [ "Resources", "md_src_2core_2README.html#autotoc_md13", null ]
          ] ],
          [ "SystemContext API", "md_src_2core_2README.html#autotoc_md14", [
            [ "Key Methods", "md_src_2core_2README.html#autotoc_md15", null ]
          ] ]
        ] ],
        [ "Usage Examples", "md_src_2core_2README.html#autotoc_md17", [
          [ "Creating Systems", "md_src_2core_2README.html#autotoc_md18", null ],
          [ "Working with Queries", "md_src_2core_2README.html#autotoc_md19", [
            [ "Basic Query", "md_src_2core_2README.html#autotoc_md20", null ],
            [ "Query with Entity Access", "md_src_2core_2README.html#autotoc_md21", null ],
            [ "Query with Filters", "md_src_2core_2README.html#autotoc_md22", null ],
            [ "Query with Manual Filtering", "md_src_2core_2README.html#autotoc_md23", null ]
          ] ],
          [ "Entity Creation and Modification", "md_src_2core_2README.html#autotoc_md24", [
            [ "Creating Entities", "md_src_2core_2README.html#autotoc_md25", null ],
            [ "Modifying Entities", "md_src_2core_2README.html#autotoc_md26", null ],
            [ "Destroying Entities", "md_src_2core_2README.html#autotoc_md27", null ]
          ] ],
          [ "Event System", "md_src_2core_2README.html#autotoc_md28", null ],
          [ "Resources and State Management", "md_src_2core_2README.html#autotoc_md29", null ],
          [ "Modules", "md_src_2core_2README.html#autotoc_md30", null ],
          [ "SubApps", "md_src_2core_2README.html#autotoc_md31", null ]
        ] ],
        [ "API Reference", "md_src_2core_2README.html#autotoc_md33", [
          [ "Core Classes", "md_src_2core_2README.html#autotoc_md34", null ],
          [ "System Context Methods", "md_src_2core_2README.html#autotoc_md35", null ],
          [ "Query Builder", "md_src_2core_2README.html#autotoc_md36", null ],
          [ "Access Policy Builder", "md_src_2core_2README.html#autotoc_md37", null ]
        ] ],
        [ "Best Practices", "md_src_2core_2README.html#autotoc_md39", [
          [ "Anti-patterns", "md_src_2core_2README.html#autotoc_md40", null ]
        ] ],
        [ "Testing", "md_src_2core_2README.html#autotoc_md42", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", null ],
        [ "Functions", "namespacemembers_func.html", null ],
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
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
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
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"_2home_2runner_2work_2HeliosEngine_2HeliosEngine_2src_2core_2include_2helios_2core_2app_2access_policy_8hpp-example.html",
"classhelios_1_1Timer.html#a4fc5c6306c82598e97ba9bed4fe4c29a",
"classhelios_1_1app_1_1FrameAllocatorResource.html#a15bd37a33960f7d348446d6d336753ea",
"classhelios_1_1app_1_1SystemContext.html#a2a9bd7fbd36af2288f1d376bc20dfe7e",
"classhelios_1_1async_1_1Executor.html",
"classhelios_1_1async_1_1Task.html#ae776e77776ea246366fc4c988d355cfb",
"classhelios_1_1ecs_1_1BasicQuery.html#a7874c3c051e8e935efdf35d30b56bbd2",
"classhelios_1_1ecs_1_1EntityCmdBuffer.html#a310cc299e00f97256e190dda19ebf80b",
"classhelios_1_1ecs_1_1World.html#a157a6b46cb9b861965d4c99e2173efff",
"classhelios_1_1ecs_1_1details_1_1Archetypes.html#a727486a0a5a9605fad719ae63786eb8e",
"classhelios_1_1ecs_1_1details_1_1DestroyEntityCmd.html#a3b6ce04169ab672ad62e356bfd2179c5",
"classhelios_1_1ecs_1_1details_1_1QueryIterator.html",
"classhelios_1_1ecs_1_1details_1_1SystemLocalStorage.html#afa8adb582e078274780e0afafbd4ffea",
"classhelios_1_1memory_1_1FrameAllocator.html#ac3bd8ea4e4e90c3a1b6984b1464c6c40",
"classhelios_1_1memory_1_1StackAllocator.html#a7d77f5cf9ee791f7cd838502ce19461f",
"classhelios_1_1utils_1_1ReverseAdapter.html#a1d48b20946337845a42b070e83d5ff9f",
"components__manager_8hpp_source.html",
"functional__adapters_8hpp.html#a1bbd3cf95b591c66c67c6b8a5f3de936",
"namespacehelios.html#a38e803d9d033acbe72b0a4d7250c104e",
"string__hash_8hpp.html",
"structhelios_1_1app_1_1details_1_1SystemInfo.html#ac3f015752756a1fa2b23dc59d82f085a",
"structhelios_1_1utils_1_1StringHash.html#a6e451328d0a9576c41ec2f90201f4b12"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';