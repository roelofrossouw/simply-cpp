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

Include the aggregate header, or an individual one (`base64.h`, `timer.h`, `date.h`, `datetime.h`, `color.h`, `geometry.h`, `ip_endpoint.h`, `ip_endpoints.h`, `console.h`, `percent.h`, `rest.h`, `ollama.h`, ...). Geometry stays part of sc-core, grouped under `geometry.h`; `rect.h` remains available for existing includes. The geometry API includes points, rectangles, rotated rectangles, polygons, circles, and DBSCAN clustering:

```cpp
#include <sc.h>

auto encoded = sc::base64::encode("Hello");

const auto clusters = sc::dbscan(std::vector<double>{1, 1.1, 20}, 0.5, 2);
```

`sc::color` reads CSS colours: `#rgb`/`#rrggbb`, `rgb()`/`rgba()`, `hsl()`/`hsla()`
and the CSS named colours (`sc::color{"rebeccapurple"}`), and prints as
`rgb()`/`rgba()`. The names come from the named-color table in
[CSS Color Module Level 4](https://www.w3.org/TR/css-color-4/#named-colors):
configuring downloads the spec's source, at most once a day, and builds the
table from it (`cmake/NamedColors.cmake`). Offline, the last list is kept, or
the committed `src/named_colors.inc` is used.

`sc::datetime` is a point in time to the second, as `sc::date` is a calendar day.
It reads and writes local time unless told otherwise (a trailing `Z` when
parsing, `utc = true` when formatting), and converts to and from Unix time:

```cpp
const auto expires = sc::datetime::from_unix(1791462896);
std::cout << expires.format() << '\n';                        // local: "2026-10-08 14:34:56" in UTC+2
std::cout << expires.format("%d %b %Y %H:%M %Z", true) << '\n'; // "08 Oct 2026 12:34 UTC"
const sc::datetime meeting{"2026-10-08T09:00:00Z"};
const auto in_a_day = sc::datetime::now() + std::chrono::hours{24};
if (meeting < in_a_day) std::cout << (in_a_day - meeting).count() << " seconds apart\n";
```

`sc::console` (`console.h`) prints plain, readable output, as the simply-cpp demos
do: a title, headings and subheadings, and each step shown as written with its
result below it. Headings are underlined and subheadings marked with `▸`; on a
terminal (unless `NO_COLOR` is set) they are bold. `SC_SHOW(expression)` and
`SC_STEP(expression)` print the code itself, commas and all:

```cpp
sc::console::title("simply-cpp redis");
sc::console::heading("A string value");
sc::console::subheading("Setting and reading it");
SC_STEP(cache.set("greeting", "Hello World!"));
SC_SHOW(cache.get("greeting"));   // prints   cache.get("greeting")
                                   //              -> "Hello World!"
sc::console::show_text("GET /hello", response_body); // a result shown as plain text
```

`sc::console::format(value)` gives the text on its own: strings quoted, `bool` as
`true`/`false`, an empty optional as `(none)`, lists as `[a, b]`, maps as
`{key: value}`, nested ones alike, anything else with `<<`. `note(text)` adds an
indented line, and `output(stream)` sends it all elsewhere than `std::cout`.

`sc::ip_endpoints` is a list of `sc::ip_endpoint`s that reads and writes the
`;`-separated form (`"redis1:6379;[::1]:6380"`). Whitespace and empty entries are
ignored, and an invalid entry throws `std::invalid_argument`. It works like a
`std::vector<sc::ip_endpoint>` (`begin()`/`end()`, `size()`, `front()`, `push_back()`,
`[]`, ...). It converts implicitly to and from both that vector and the string
form, so `sc::redis` and `sc::postgres`, which take an `sc::ip_endpoints`, accept a
string, a vector or a braced list. Don't overload a function on both
`std::string` and `std::vector<sc::ip_endpoint>`: an `sc::ip_endpoints` argument would
be ambiguous. Use `to_string(separator)` for another separator:

```cpp
sc::ip_endpoints servers{"redis1;redis2:7000", 6379};   // default port 6379
servers.push_back({"redis3", 7001});
for (const auto &server : servers) std::cout << server << '\n';

sc::redis cache{servers};
const std::string text = servers;                       // "redis1:6379;redis2:7000;redis3:7001"
const auto bootstrap = servers.to_string(",");          // for Kafka's bootstrap.servers
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

`sc-core-demo` is a short tour of the basics: base64, dates and times, server
endpoints, and strings and the environment. Each line shows a call, as written,
and what it returned. It is installed with the runtime package (`simply-cpp`),
so it also shows an installation works without the `-dev` package, and needs no
server. Every module has a demo like it (`sc-<module>-demo`); they demonstrate,
they aren't tests, so CTest doesn't run them.

sc-core has a closer look at each area too, installed alongside it:

| Demo | Shows |
|---|---|
| `sc-core-geometry` | points and sizes, rectangles (overlap, distance, grouping), rotated rectangles and polygons, DBSCAN clustering |
| `sc-core-time` | `sc::date` calendar arithmetic, `sc::datetime` and Unix time, `sc::timer` |
| `sc-core-text` | UTF-8 measuring, slicing and repair, base64, `explode`, `getenv`, files |
| `sc-core-numbers` | `sc::percent`, `sc::color`, `sc::matrix`, dual numbers for derivatives |
| `sc-core-network` | `ip_address`, `sc::ip_endpoint` and `sc::ip_endpoints` |

Its source is `examples/sc-core-demo.cpp`; the code below is copied from it at
configure time, so it always matches code that compiles:

<!-- sc-example: examples/sc-core-demo.cpp -->
```cpp
sc::console::heading("Base64");
SC_SHOW(sc::base64::encode("Hello World!"));
SC_SHOW(sc::base64::decode("SGVsbG8gV29ybGQh"));

sc::console::heading("Dates and times");
SC_SHOW(sc::date{"2026-01-31"}.add(1, "M"));                         // calendar arithmetic
SC_SHOW(sc::datetime{"2026-10-09 14:30:00"}.add(90, "i").format()); // 90 minutes later
SC_SHOW(sc::datetime::from_unix(0).format("%d %b %Y %H:%M %Z", true));

sc::console::heading("Server endpoints");
SC_SHOW(sc::ip_endpoints("redis1;redis2:6380", 6379)); // ';'-separated, with a default port
SC_SHOW(sc::ip_endpoint::parse("[::1]:5432").host);

sc::console::heading("Strings and the environment");
SC_SHOW(sc::explode("a;b;;c"));
SC_SHOW(sc::getenv("HOME", "(not set)"));
```
<!-- /sc-example -->

Demos in other modules that talk to a server read it from
`SC_<MODULE>_DEMO_SERVER`: one or more `host[:port]` / `[ipv6]:port` entries
separated by `;` (quote it in a shell). Unset or empty means
`127.0.0.1:<default port>`; an invalid value is an error:

```cpp
const sc::ip_endpoints servers{sc::getenv("SC_REDIS_DEMO_SERVER", "127.0.0.1"), 6379};
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
