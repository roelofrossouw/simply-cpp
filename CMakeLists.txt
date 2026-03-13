if (APPLE)
    cmake_minimum_required(VERSION 3.30)
else ()
    cmake_minimum_required(VERSION 3.10)
endif ()


## External Dependencies ##
# git remote add base64 https://github.com/tobiaslocker/base64.git
# git subtree add --prefix=third_party/base64 base64 master --squash
set(BASE64_ENABLE_TESTING OFF)
add_subdirectory(third_party/base64 base64 EXCLUDE_FROM_ALL)

## LIBRARIES ##

include_directories("src")

project(rjr-base64)
add_library(${PROJECT_NAME} STATIC src/base64.cpp src/base64.h)
target_link_libraries(${PROJECT_NAME} base64)

# Combined libary for easy usage
project(rjr)
add_library(${PROJECT_NAME} SHARED src/rjr.cpp src/rjr.h)
target_link_libraries(${PROJECT_NAME} rjr-base64)

## TESTS ##

project(test_base64)
add_executable(${PROJECT_NAME} test/base64.cpp
        src/rjr.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE rjr)

