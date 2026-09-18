# simply-cpp

Library that wraps common C++ libraries behind one simple, consistent API.

The idea of this library is to make it easier to use C++, especially for those who are new to C++.
The concept is to create wrappers around existing libraries, not to implement them from scratch.
This should keep maintenance to the minimum.

In my opinion, ease of use means that there should not be a need to maintain state or have a lot of work to do for setting up.
All functionality is run from a global or static function if no state is required. Otherwise by creating an object with a simple constructor and methods.
Most objects should be able to be streamed as a string (e.g. `cout << obj`).

Example for a static function of the `base64` class:

```cpp
sc::base64::encode("Hello World!");
```

Example for using a simple class:

```cpp
sc::timer t, t2;
// Do some work
cout << "Some work took " << t << endl;
t2.reset();
// Do more work
t.stop();
t2.stop();
// Some cleanup that we don't care about the timing
cout << "More work took " << t2 << endl;
cout << "Some work and more work together took " << t << endl;
```

## Factors considered to keep things simple

1. Headers should not include third party headers so that users don't have issues with needing 3rd party headers or include paths (only use forward declarations).
1. Headers should not affect the global namespace.
1. Headers should not add namespace usages.
1. Headers should include comments to document the public methods.

## Install

### Homebrew (macOS)

```bash
brew tap roelofrossouw/sc
brew install simply-cpp
```

### apt (Ubuntu)

```bash
sudo curl -fsSL https://apt.roelof.co.za/setup.sh | bash
sudo apt -y install simply-cpp-dev
```

### CMake FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(
        sc-core
        GIT_REPOSITORY https://github.com/roelofrossouw/simply-cpp.git
        GIT_TAG main # or a specific tag, e.g. v1.1.9, to stay stable
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(sc-core)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE sc::sc-core)
```

### Git submodule

```bash
git submodule add https://github.com/roelofrossouw/simply-cpp.git third_party/sc-core
```

```cmake
add_subdirectory(third_party/sc-core)
target_link_libraries(myapp PRIVATE sc::sc-core)
```

## Dependencies

sc-core is the base of the suite - it doesn't depend on any other `sc-*` module. It does use:

- **libcurl** - a system dependency (`libcurl4-openssl-dev` on apt, `curl` on brew); installed automatically if missing when building from source.
- **nlohmann_json** - fetched and built from source automatically; nothing to install for it.

## Usage

```cmake
find_package(sc-core CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE sc::sc-core)
```

Include the aggregate header, or any of the individual ones (`base64.h`, `timer.h`, `date.h`, `color.h`, `rect.h`, `percent.h`, `rest.h`, `ollama.h`, ...):

```cpp
#include <sc.h>

auto encoded = sc::base64::encode("Hello");
```

## Requirements

- CMake 3.22 or newer
- A C++20 compiler (the library uses concepts, so C++17 is not enough)

## Building and testing

```bash
cmake -B build -S .
cmake --build build -j
ctest --test-dir build --output-on-failure
```
