# simply-cpp

Library to simplify a variety of C++ libraries into a standard usage methodology.

The idea of this library is to make it easier to use C++, especially for those who are new to C++.

Every library can be used on its own, or you can include the common library to include everything to get going.

## Usage

### CMake

1. You can use "FetchContent" to include the library in your CMake project:

```cmake
include(FetchContent)
FetchContent_Declare(
        scpp
        GIT_REPOSITORY https://github.com/roelofrossouw/simply-cpp.git
        GIT_TAG origin/main # Or a specific tag to stay stable #
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(scpp)

add_executable(trysc main.cpp)
target_link_libraries(trysc scpp)
```

2. Or you can simply clone the repository and then include the library in your CMake project:

```bash
# If your project is a git repo, then
git remote add scpp https://github.com/roelofrossouw/simply-cpp.git
git subtree add --prefix=third_party/scpp scpp master --squash
# otherwise you can clone it
git clone https://github.com/roelofrossouw/simply-cpp.git third_party/scpp
```

```cmake
add_subdirectory(third_party/scpp scpp EXCLUDE_FROM_ALL)
```

Once you have simply-cpp included in your project include the header file:

```cpp
#include "sc.h"
```

And access any part of the library:

```cpp
auto encoded = sc::base64::encode("Hello");
```