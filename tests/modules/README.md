# Module Tests

This directory contains tests for Helios Engine modules. The test infrastructure provides automatic discovery and simplified test creation for custom modules.

## Directory Structure

```
tests/modules/
├── README.md                           # This file
├── CMakeLists.txt                      # Automatic test discovery
└── {module_name}/                      # Tests for each module
    ├── CMakeLists.txt                  # Module test configuration
    ├── unit/                           # Unit tests
    │   ├── main.cpp                    # Test runner entry point
    │   └── {module_name}_test.cpp      # Test cases
    └── integration/                    # Integration tests (optional)
        ├── main.cpp                    # Test runner entry point
        └── {module_name}_integration_test.cpp
```

## Quick Start

### 1. Create Test Directory Structure

```bash
# From project root
mkdir -p tests/modules/your_module/{unit,integration}
```

### 2. Create CMakeLists.txt

Create `tests/modules/your_module/CMakeLists.txt`:

```cmake
# Unit tests
helios_add_module_test(
    MODULE_NAME your_module
    TYPE unit
    SOURCES
        unit/main.cpp
        unit/your_module_test.cpp
)

# Integration tests (optional)
helios_add_module_test(
    MODULE_NAME your_module
    TYPE integration
    SOURCES
        integration/main.cpp
        integration/your_module_integration_test.cpp
)
```

### 3. Create Test Sources

**unit/main.cpp:**

```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
```

**unit/your_module_test.cpp:**

```cpp
#include <doctest/doctest.h>
#include <helios/your_module/your_module.hpp>

TEST_CASE("YourModule - Basic Test") {
    // Your test code here
    CHECK(true);
}
```

### 4. Build and Run Tests

```bash
# Build with tests enabled
python scripts/build.py --tests

# Run all tests
ctest

# Run only your module's tests
ctest -R your_module

# Run only unit tests
ctest -L unit

# Run with verbose output
ctest --verbose --output-on-failure
```

## Test Infrastructure

### Automatic Module Discovery

The `tests/modules/CMakeLists.txt` automatically discovers and adds tests for all enabled modules. It:

1. Scans `src/modules/` for module directories
2. Checks if each module is enabled via `HELIOS_BUILD_{MODULE_NAME}_MODULE`
3. Looks for corresponding test directories in `tests/modules/{module_name}`
4. Includes the module's test configuration if found

This means you only need to create your test directory and CMakeLists.txt - no manual registration required!

### helios_add_module_test() Function

The `helios_add_module_test()` function simplifies test creation by:

- Automatically checking if the module is enabled
- Linking to `helios::module::{module_name}` automatically
- Following consistent naming conventions
- Creating proper test labels for filtering

**Parameters:**

- `MODULE_NAME` (required): Name of your module (e.g., "example", "physics")
- `TYPE` (required): Either "unit" or "integration"
- `SOURCES` (required): List of test source files
- `DEPENDENCIES` (optional): Additional libraries to link

**Example:**

```cmake
helios_add_module_test(
    MODULE_NAME physics
    TYPE unit
    SOURCES
        unit/main.cpp
        unit/collision_test.cpp
        unit/rigidbody_test.cpp
    DEPENDENCIES
        some_external_lib  # Optional
)
```

### Test Target Naming

Tests follow this naming convention:

- Target name: `helios_module_{module_name}_{type}`
- Examples:
  - `helios_module_example_unit`
  - `helios_module_physics_integration`

### Test Labels

Each test is automatically labeled with:

- Module name: `module_{module_name}` (e.g., "module_example")
- Test type: `unit` or `integration`

Use labels to run specific test categories:

```bash
# All unit tests
ctest -L unit

# All integration tests
ctest -L integration

# All tests for a specific module
ctest -L module_example
```

## Writing Tests

### Unit Tests

Unit tests focus on testing individual components, functions, or classes in isolation.

```cpp
#include <doctest/doctest.h>
#include <helios/your_module/component.hpp>

TEST_SUITE("YourModule - Components") {
    TEST_CASE("Component default construction") {
        your_module::SomeComponent comp;
        CHECK_EQ(comp.value, 0);
    }

    TEST_CASE("Component with parameters") {
        your_module::SomeComponent comp{42};
        CHECK_EQ(comp.value, 42);
    }
}
```

### Integration Tests

Integration tests verify that components work together correctly with the app and ECS system.

```cpp
#include <doctest/doctest.h>
#include <helios/your_module/your_module.hpp>
#include <helios/core/app/app.hpp>

TEST_SUITE("YourModule - Integration") {
    TEST_CASE("Module integrates with App") {
        helios::app::App app;
        app.AddModule<your_module::YourModule>();

        // Run update cycle
        app.Update();

        CHECK(true);  // Verify no crashes
    }
}
```

### doctest Assertions

Use specialized macros for better diagnostics:

```cpp
// Equality
CHECK_EQ(actual, expected);     // ==
CHECK_NE(actual, expected);     // !=

// Comparisons
CHECK_LT(actual, expected);     // <
CHECK_LE(actual, expected);     // <=
CHECK_GT(actual, expected);     // >
CHECK_GE(actual, expected);     // >=

// Boolean
CHECK(condition);
CHECK_FALSE(condition);

// Floating point
CHECK_EQ(actual, doctest::Approx(expected));

// Exceptions (if enabled)
CHECK_NOTHROW(expression);
```

## Example: Complete Module Test Setup

See `tests/modules/example/` for a complete reference implementation:

- `CMakeLists.txt` - Test configuration
- `unit/main.cpp` - Unit test entry point
- `unit/example_test.cpp` - Component, resource, and system unit tests
- `integration/main.cpp` - Integration test entry point
- `integration/example_integration_test.cpp` - Full app integration tests

## Running Tests

### Command Line

```bash
# All tests
ctest

# Parallel execution
ctest --parallel 4

# Verbose output
ctest --verbose

# Show output only on failure
ctest --output-on-failure

# Stop on first failure
ctest --stop-on-failure

# Run specific test
ctest -R helios_module_example_unit

# Run by label
ctest -L unit
ctest -L integration
ctest -L module_example

# With timeout
ctest --timeout 60
```

### Make Targets

```bash
# Build all tests
make BUILD_TYPE=debug TESTS=1

# Run tests
make test BUILD_TYPE=debug
```

### Via Build Script

```bash
# Build with tests
python scripts/build.py --tests

# Then run from build directory
cd build/debug/linux  # or your platform
ctest --output-on-failure
```

## Best Practices

### 1. Test Organization

- **Unit tests**: Test individual components, functions, classes
- **Integration tests**: Test module integration with App and other systems
- Keep tests focused and independent

### 2. Test Naming

Use descriptive test names that explain what is being tested:

```cpp
TEST_CASE("PhysicsComponent calculates velocity correctly") { ... }
TEST_CASE("RigidBody responds to gravity") { ... }
```

### 3. Test Suites

Group related tests into suites:

```cpp
TEST_SUITE("YourModule - Components") {
    // Component tests
}

TEST_SUITE("YourModule - Systems") {
    // System tests
}

TEST_SUITE("YourModule - Integration") {
    // Integration tests
}
```

### 4. Setup and Teardown

Use doctest's subcases for shared setup:

```cpp
TEST_CASE("Complex test with setup") {
    // Shared setup
    helios::app::App app;
    app.AddModule<YourModule>();

    SUBCASE("Scenario 1") {
        // Test scenario 1
    }

    SUBCASE("Scenario 2") {
        // Test scenario 2
    }

    // Shared teardown happens automatically
}
```

### 5. Test Data

- Use constants for magic numbers
- Create helper functions for common test data
- Keep test data minimal and focused

### 6. Testing Guidelines

- Each test should be independent
- Tests should be deterministic
- Avoid testing implementation details
- Focus on public API behavior
- Use meaningful assertions

## Troubleshooting

### Module Tests Not Found

If your module tests aren't being discovered:

1. Check that your module is enabled: `HELIOS_BUILD_{MODULE_NAME}_MODULE=ON`
2. Verify test directory exists: `tests/modules/{module_name}/`
3. Check that `CMakeLists.txt` exists in your test directory
4. Ensure module target exists: `helios::module::{module_name}`

### Build Errors

If tests fail to build:

1. Check that you're including the correct headers: `#include <helios/{module_name}/...>`
2. Verify module dependencies are correctly specified
3. Check that your module is built before tests

### Test Failures

If tests fail unexpectedly:

1. Run with verbose output: `ctest --verbose`
2. Run specific failing test: `ctest -R test_name --verbose`
3. Use debugger: `gdb ./bin/helios_module_{name}_{type}`

## See Also

- [Creating Custom Modules Guide](../../docs/guides/creating-modules.md)
- [Core Tests](../core/) - Examples of comprehensive test coverage
- [CMake TestUtils](../../cmake/TestUtils.cmake) - Test function documentation
- [doctest Documentation](https://github.com/doctest/doctest) - Testing framework documentation
