# simply-cpp

Library to simplify a variety of C++ libraries into a standard usage methodology.

The idea of this library is to make it easier to use C++, especially for those who are new to C++.

Every library can be used on its own, or you can include the common library to include everything to get going.

## Usage

To use simply-cpp, include the main or individual header files, library in your project and start using the functions and classes provided by the library.

For example, to use the common library, include the following line in your project:

```cpp
#include "simply-cpp/sc.hpp"
```

This will include all the commonly used functions and classes from the library.
You can then use functions e.g. base64 encoding like this:

```cpp
auto encoded = sc::base64::encode("Hello");
```