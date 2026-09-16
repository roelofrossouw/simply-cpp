# Tests

Notes for working on simply-cpp itself. The top level README is for people using the
library; nothing here is of any interest to them.

## Running

```bash
cmake -B build -S .
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Tests are built when simply-cpp is the top level project. As a subproject they are
off, and `-DSC_CORE_BUILD_TESTS=ON` turns them back on.

`--test-dir build` picks up every test through the top level `CTestTestfile.cmake`.
Point it at `build/sc-test` instead to work in the test directory itself.

### Labels

`rest` and `ollama` reach outside the build – a live http request and a local ollama
server. Everything else is self contained and labelled `unit`:

```bash
ctest --test-dir build -LE network      # skip anything that needs the outside world
ctest --test-dir build -R test-rect     # one test by name
```

`-LE network` is what CI runs.

Neither network test fails when there is nothing to answer. They check as much as
they can offline – a local file fetch, the fetch cache, a refused connection,
defaults and setters – then say the live part did not run and pass on what they did
check. A test that needs a service is only worth writing if it degrades this way.

## Adding a test

Drop `something.cpp` in this directory and register it in `CMakeLists.txt`:

```cmake
add_sc_test(something LABELS unit)
```

That builds `test-something` and hands it to ctest. All arguments are optional:

| Argument | Default |
|---|---|
| `LABELS <label>...` | none – use `unit`, or `network` if it needs the outside world |
| `TIMEOUT <seconds>` | 120 |
| `LINK_LIBRARIES <lib>...` | `${SC_TEST_LINK_LIBRARIES}`, set to `sc` at the top of `CMakeLists.txt` |

Tests run with their working directory set to the build directory, so a test needing
fixtures reads them from `resource/`. That directory is copied next to the binaries by
the `copy_cs_resources` target; depend on it explicitly:

```cmake
add_dependencies(test-something copy_cs_resources)
```

## sc_test.h

`assert()` is not used here. It compiles out of release builds, and it stops at the
first failure, so a broken change shows you one problem at a time. The macros in
`sc_test.h` stay active in every build, and a failed check reports its file, line,
section and both values and then carries on, so one run shows everything that broke:

```
-- Adding a scalar inflates the rect on every side
rect.cpp:8: FAILED [Adding a scalar inflates the rect on every side]: r.width() == width
    lhs = 50.5
    rhs = 99
FAILED: 1/2 checks passed.
```

A test is a plain `main()` that ends in `TEST_SUMMARY()`, which prints the tally and
returns the exit code:

```cpp
#include <sc.h>
#include "sc_test.h"

int main() {
    SECTION("Encoding");
    CHECK_EQ(sc::base64::encode("f"), std::string{"Zg=="});
    CHECK_THROWS_AS(sc::base64::decode("A"), std::runtime_error);
    TEST_SUMMARY();
}
```

| Macro | Checks |
|---|---|
| `CHECK(expr)` | expr is true |
| `CHECK_MSG(expr, message)` | expr is true, reporting your own message on failure |
| `CHECK_EQ(a, b)`, `CHECK_NE(a, b)`, `CHECK_LT(a, b)` | a comparison, printing both sides |
| `CHECK_NEAR(a, b, tolerance)` | two floating point values within a tolerance |
| `CHECK_BETWEEN(value, low, high)` | an inclusive range, for timings and the like |
| `CHECK_THROWS_AS(expr, type)` | expr throws that exception type |
| `CHECK_NOTHROW(expr)` | expr throws nothing |
| `SECTION(name)` | nothing – labels the checks that follow |
| `TEST_SUMMARY()` | nothing – prints the tally and returns the exit code |

Two things to watch for:

- A braced initialiser containing a comma splits a macro argument in two:
  `CHECK_EQ(sc::percent{0.8, 0}, x)` will not compile. Assign to a local first.
- `CHECK_EQ` prints both sides through `operator<<` when the type has one that
  ordinary lookup or ADL can find from `sc_test.h`. For a type whose `operator<<`
  the test defines itself, declare it before including `sc_test.h` – see the top of
  `color.cpp` – otherwise failures print `<not printable>` and the check still works.

### Known issues

A test that documents a real defect is left in place, commented out, with the
expected values written out and the cause named:

```cpp
// KNOWN ISSUE: a negative scalar still runs into the clamp ...
// SECTION("A negative scalar reverses the direction");
// {
//     sc::rect inflated{10, 10, 30, 30};
//     inflated -= -5;
//     check_edges(inflated, 5.0, 5.0, 40.0, 40.0);
// }
```

That keeps the suite green while leaving the next person the finished test to
uncomment once the bug is fixed, rather than an assertion that quietly pins the
wrong behaviour in place.

## Other modules

`sc_test.h` is installed with the library, and `add_sc_test` comes from
`SimplyCppFunctions.cmake` through `find_package(sc)`, so the other simply-cpp
modules use both without keeping copies. Anything added here is available to them
once core is reinstalled.
