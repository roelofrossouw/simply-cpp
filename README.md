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
curl -fsSL https://apt.roelof.co.za/setup.sh | bash # taps roelofrossouw/sc - same command as the apt one below
brew install simply-cpp
```

### apt (Ubuntu)

```bash
curl -fsSL https://apt.roelof.co.za/setup.sh | bash # registers the apt repo - same command as the brew one above
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

Include the aggregate header, or an individual one (`base64.h`, `timer.h`, `date.h`, `color.h`, `geometry.h`, `ip_endpoint.h`, `demo_servers.h`, `percent.h`, `rest.h`, `ollama.h`, ...). Geometry stays part of sc-core, grouped under `geometry.h`; `rect.h` remains available for existing includes. The geometry API includes points, rectangles, rotated rectangles, polygons, circles, and DBSCAN clustering:

```cpp
#include <sc.h>

auto encoded = sc::base64::encode("Hello");

const auto clusters = sc::dbscan(std::vector<double>{1, 1.1, 20}, 0.5, 2);
```

`core.h` has small PHP-style helpers: `file_get_contents()`, `file_put_contents()`,
`basename()`, `getenv()` and `explode()`. `sc::getenv()` returns the fallback when a
variable is unset or empty. `sc::explode()` splits on a separator (default `;`),
keeping empty items like PHP does:

```cpp
const auto topics = sc::explode(sc::getenv("TOPICS", "a;b"));   // {"a", "b"}
const auto parts = sc::explode("x,,y", ",");                     // {"x", "", "y"}
```

## Demo

`sc-core-demo` is installed with the runtime package (`simply-cpp`), so you can
check an installation works without the `-dev` package. It needs no server.
Its source is `examples/sc-core-demo.cpp`; the code below is copied from it at
configure time, so it always matches code that compiles:

<!-- sc-example: examples/sc-core-demo.cpp -->
```cpp
sc::timer sw;
const std::string sample = "Hello World!";
const auto encoded = sc::base64::encode(sample);
const auto decoded = sc::base64::decode(encoded);
std::cout << sample << " => " << encoded << " => " << decoded << '\n';
std::cout << "Done after " << sw << '\n';
```
<!-- /sc-example -->

Demos in other modules that talk to a server read it from
`SC_<MODULE>_DEMO_SERVER` with `sc::demo_servers()` (`demo_servers.h`). The value
is one or more `host[:port]` / `[ipv6]:port` entries separated by `;` (quote it
in a shell). Unset, empty or invalid falls back to `127.0.0.1:<default port>`:

```cpp
const auto servers = sc::demo_servers("SC_REDIS_DEMO_SERVER", 6379);
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
