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
project(scpp-base64)
add_library(${PROJECT_NAME} STATIC src/base64.cpp include/base64.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} base64)

# Combined libary for easy usage
project(scpp VERSION 1.0.0 DESCRIPTION "simply-cpp common shared library")
add_library(${PROJECT_NAME} SHARED src/sc.cpp sc.h)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC scpp-base64)

# Combined static libary for easy usage
project(scpp-static VERSION 1.0.0 DESCRIPTION "simply-cpp common static library")
add_library(${PROJECT_NAME} STATIC src/sc.cpp sc.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC scpp-base64)


## TESTS ##

project(test_base64)
add_executable(${PROJECT_NAME} test/base64.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE scpp)

