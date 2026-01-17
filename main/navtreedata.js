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
      [ "Table of Contents", "index.html#autotoc_md60", null ],
      [ "About The Project", "index.html#autotoc_md62", [
        [ "Key Features", "index.html#autotoc_md63", null ],
        [ "Design Philosophy", "index.html#autotoc_md64", null ]
      ] ],
      [ "Architecture", "index.html#autotoc_md66", [
        [ "Entity Component System (ECS)", "index.html#autotoc_md67", [
          [ "<strong>Entities</strong>", "index.html#autotoc_md68", null ],
          [ "<strong>Components</strong>", "index.html#autotoc_md69", null ],
          [ "<strong>Systems</strong>", "index.html#autotoc_md70", null ],
          [ "<strong>World</strong>", "index.html#autotoc_md71", null ],
          [ "<strong>Scheduling</strong>", "index.html#autotoc_md72", null ]
        ] ],
        [ "Modular Design", "index.html#autotoc_md73", null ],
        [ "Creating Custom Modules", "index.html#autotoc_md74", null ]
      ] ],
      [ "Getting Started", "index.html#autotoc_md76", [
        [ "Requirements", "index.html#autotoc_md77", null ],
        [ "Dependencies", "index.html#autotoc_md78", [
          [ "Core Dependencies", "index.html#autotoc_md79", null ],
          [ "Test Dependencies", "index.html#autotoc_md80", null ],
          [ "Installation Methods", "index.html#autotoc_md81", null ]
        ] ],
        [ "Quick Start", "index.html#autotoc_md82", [
          [ "1. Clone the Repository", "index.html#autotoc_md83", null ],
          [ "2. Install Dependencies", "index.html#autotoc_md84", null ],
          [ "3. Configure the Project", "index.html#autotoc_md85", null ],
          [ "4. Build the Project", "index.html#autotoc_md86", null ],
          [ "5. Run Tests", "index.html#autotoc_md87", null ]
        ] ],
        [ "Build Options", "index.html#autotoc_md88", [
          [ "Using Makefile (Recommended)", "index.html#autotoc_md89", null ],
          [ "Using Python Scripts Directly", "index.html#autotoc_md90", null ],
          [ "Using CMake Presets", "index.html#autotoc_md91", null ]
        ] ]
      ] ],
      [ "Usage Examples", "index.html#autotoc_md93", [
        [ "Basic Application Setup", "index.html#autotoc_md94", null ],
        [ "Creating Entities", "index.html#autotoc_md95", null ],
        [ "Querying Entities", "index.html#autotoc_md96", null ],
        [ "Using Modules", "index.html#autotoc_md97", null ]
      ] ],
      [ "Core Module", "index.html#autotoc_md99", [
        [ "Overview", "index.html#autotoc_md100", null ]
      ] ],
      [ "Roadmap", "index.html#autotoc_md102", [
        [ "Completed", "index.html#autotoc_md103", null ],
        [ "In Progress", "index.html#autotoc_md104", [
          [ "<strong>Runtime modules</strong>", "index.html#autotoc_md105", null ],
          [ "<strong>Renderer modules</strong>", "index.html#autotoc_md106", null ]
        ] ]
      ] ],
      [ "Acknowledgments", "index.html#autotoc_md108", null ],
      [ "License", "index.html#autotoc_md110", null ],
      [ "Contact", "index.html#autotoc_md112", null ],
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
    [ "Example Module", "md_src_2modules_2example_2README.html", [
      [ "Overview", "md_src_2modules_2example_2README.html#autotoc_md45", null ],
      [ "Build Option", "md_src_2modules_2example_2README.html#autotoc_md46", null ],
      [ "Directory Structure", "md_src_2modules_2example_2README.html#autotoc_md47", null ],
      [ "Usage", "md_src_2modules_2example_2README.html#autotoc_md48", [
        [ "Adding the Module to Your App", "md_src_2modules_2example_2README.html#autotoc_md49", null ],
        [ "Using the Example Component", "md_src_2modules_2example_2README.html#autotoc_md50", null ],
        [ "Querying the Example Resource", "md_src_2modules_2example_2README.html#autotoc_md51", null ]
      ] ],
      [ "Components", "md_src_2modules_2example_2README.html#autotoc_md52", [
        [ "ExampleComponent", "md_src_2modules_2example_2README.html#autotoc_md53", null ]
      ] ],
      [ "Resources", "md_src_2modules_2example_2README.html#autotoc_md54", [
        [ "ExampleResource", "md_src_2modules_2example_2README.html#autotoc_md55", null ]
      ] ],
      [ "Systems", "md_src_2modules_2example_2README.html#autotoc_md56", [
        [ "ExampleSystem", "md_src_2modules_2example_2README.html#autotoc_md57", null ]
      ] ],
      [ "Creating Your Own Module", "md_src_2modules_2example_2README.html#autotoc_md58", null ]
    ] ],
    [ "creating-modules", "md_docs_2guides_2creating-modules.html", [
      [ "Creating Custom Modules", "md_docs_2guides_2creating-modules.html#autotoc_md113", [
        [ "Table of Contents", "md_docs_2guides_2creating-modules.html#autotoc_md114", null ],
        [ "Overview", "md_docs_2guides_2creating-modules.html#autotoc_md116", null ],
        [ "Module Structure", "md_docs_2guides_2creating-modules.html#autotoc_md117", [
          [ "File Descriptions", "md_docs_2guides_2creating-modules.html#autotoc_md118", null ]
        ] ],
        [ "Quick Start", "md_docs_2guides_2creating-modules.html#autotoc_md120", null ],
        [ "Step-by-Step Guide", "md_docs_2guides_2creating-modules.html#autotoc_md122", [
          [ "1. Create Directory Structure", "md_docs_2guides_2creating-modules.html#autotoc_md123", null ],
          [ "2. Create Module.cmake (Optional)", "md_docs_2guides_2creating-modules.html#autotoc_md124", null ],
          [ "3. Create CMakeLists.txt", "md_docs_2guides_2creating-modules.html#autotoc_md125", null ],
          [ "4. Implement Your Module", "md_docs_2guides_2creating-modules.html#autotoc_md126", null ]
        ] ],
        [ "CMake Functions Reference", "md_docs_2guides_2creating-modules.html#autotoc_md128", [
          [ "helios_register_module", "md_docs_2guides_2creating-modules.html#autotoc_md129", null ],
          [ "helios_add_module", "md_docs_2guides_2creating-modules.html#autotoc_md130", null ],
          [ "helios_define_module", "md_docs_2guides_2creating-modules.html#autotoc_md131", null ]
        ] ],
        [ "Module Dependencies", "md_docs_2guides_2creating-modules.html#autotoc_md133", [
          [ "Depending on Other Helios Modules", "md_docs_2guides_2creating-modules.html#autotoc_md134", null ],
          [ "Depending on External Libraries", "md_docs_2guides_2creating-modules.html#autotoc_md135", null ],
          [ "Conditional Dependencies", "md_docs_2guides_2creating-modules.html#autotoc_md136", null ]
        ] ],
        [ "Build Options", "md_docs_2guides_2creating-modules.html#autotoc_md138", [
          [ "Using Build Options", "md_docs_2guides_2creating-modules.html#autotoc_md139", null ],
          [ "Querying Module Status", "md_docs_2guides_2creating-modules.html#autotoc_md140", null ]
        ] ],
        [ "Best Practices", "md_docs_2guides_2creating-modules.html#autotoc_md142", [
          [ "1. Use Consistent Naming", "md_docs_2guides_2creating-modules.html#autotoc_md143", null ],
          [ "2. Organize Headers Properly", "md_docs_2guides_2creating-modules.html#autotoc_md144", null ],
          [ "3. Minimize Public Dependencies", "md_docs_2guides_2creating-modules.html#autotoc_md145", null ],
          [ "4. Document Your Module", "md_docs_2guides_2creating-modules.html#autotoc_md146", null ],
          [ "5. Write Tests", "md_docs_2guides_2creating-modules.html#autotoc_md147", null ],
          [ "6. Use Precompiled Headers for Large Modules", "md_docs_2guides_2creating-modules.html#autotoc_md148", null ]
        ] ],
        [ "Examples", "md_docs_2guides_2creating-modules.html#autotoc_md150", [
          [ "Minimal Module", "md_docs_2guides_2creating-modules.html#autotoc_md151", null ],
          [ "Module with Dependencies", "md_docs_2guides_2creating-modules.html#autotoc_md152", null ],
          [ "Header-Only Module", "md_docs_2guides_2creating-modules.html#autotoc_md153", null ],
          [ "Module with Optional Features", "md_docs_2guides_2creating-modules.html#autotoc_md154", null ]
        ] ],
        [ "Troubleshooting", "md_docs_2guides_2creating-modules.html#autotoc_md156", [
          [ "Module Not Found", "md_docs_2guides_2creating-modules.html#autotoc_md157", null ],
          [ "Dependency Errors", "md_docs_2guides_2creating-modules.html#autotoc_md158", null ],
          [ "Build Order Issues", "md_docs_2guides_2creating-modules.html#autotoc_md159", null ]
        ] ],
        [ "See Also", "md_docs_2guides_2creating-modules.html#autotoc_md161", null ]
      ] ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
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
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ],
    [ "Examples", "examples.html", "examples" ]
  ] ]
];

var NAVTREEINDEX =
[
"Library-example.html",
"classhelios_1_1Timer.html",
"classhelios_1_1app_1_1App.html#ad00f452ada02721db7eae955b0eba1f5",
"classhelios_1_1app_1_1SubApp.html#a97e261a8fdad2b345bf2bb019ce96aa5",
"classhelios_1_1app_1_1details_1_1ScheduleExecutor.html#afae39b7d3dde1aa34de8e70a3c0ec040",
"classhelios_1_1async_1_1SubTaskGraph.html#a61b89f007aca8810fce649b351874b32",
"classhelios_1_1container_1_1BasicStaticString.html#a45080898c8dd518da06ac2181ab5e1cd",
"classhelios_1_1container_1_1SparseSet.html#a7576763f28bd88882b37844a8adc4281",
"classhelios_1_1ecs_1_1BasicQueryWithEntity.html#ab44e72f4411e116eaca5e31977ac8adc",
"classhelios_1_1ecs_1_1EventReader.html#a877068c4eafce5231660a5606ced9724",
"classhelios_1_1ecs_1_1details_1_1AddComponentCmd.html#a55151e390c7d96d0c4fc673252ccc1e9",
"classhelios_1_1ecs_1_1details_1_1ComponentStorage.html#a5b21df2c686e7859de700c6b6f3abfad",
"classhelios_1_1ecs_1_1details_1_1EventQueue.html#af38bf78014d3593b7004d8ca42e9c39d",
"classhelios_1_1ecs_1_1details_1_1ResourceStorage.html#a4ddaa0c549600ba3530d2ac535c1f9ba",
"classhelios_1_1memory_1_1ArenaAllocator.html",
"classhelios_1_1memory_1_1NFrameAllocator.html#ae7fe715f5eba83236e7b1d86d65b8300",
"classhelios_1_1utils_1_1DynamicLibrary.html#a4076825b05ec3879b5177bb37bff236d",
"classhelios_1_1utils_1_1MapAdapter.html#a14e07d6291a03d265feed573a0e37195",
"classhelios_1_1utils_1_1StepByAdapter.html#a7ad930037fb9e06f61f0d11a61f55af2",
"concepthelios_1_1ecs_1_1ComponentWithNameTrait.html",
"functional__adapters_8hpp.html",
"md_src_2core_2README.html#autotoc_md36",
"namespacehelios_1_1utils.html#a2497009418a7ac12677b3dfccf2afb90",
"structhelios_1_1app_1_1CleanUp.html#af0c9538f30e592f31ce723922c502b66",
"structhelios_1_1app_1_1details_1_1SystemOrdering.html",
"structhelios_1_1memory_1_1AllocatorStats.html#a313c2b1c8f3b7478190b86cb7b53c869"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';