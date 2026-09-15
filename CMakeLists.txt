cmake_minimum_required(VERSION 3.22)
project(simply-cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options:
option(SC_BUILD_TESTS "Build simply-cpp tests" ${PROJECT_IS_TOP_LEVEL})

find_package(Git QUIET)
if (Git_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE SC_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE GIT_RESULT
    )

    if (GIT_RESULT EQUAL 0)
        string(REGEX REPLACE "^v" "" SC_VERSION "${SC_VERSION}")
    else ()
        set(SC_VERSION "1.1.0")
    endif ()
else ()
    set(SC_VERSION "1.1.0")
endif ()

message(STATUS "SC Version: ${SC_VERSION}")
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)

find_package(CURL QUIET)
if (NOT CURL_FOUND)
    if (UNIX AND EXISTS "/usr/bin/apt")
        message(STATUS "libcurl not found, attempting installation...")
        execute_process(COMMAND sudo apt -y install libcurl4-openssl-dev RESULT_VARIABLE INSTALL_RESULT)
    endif ()
    if (APPLE)
        execute_process(COMMAND brew install curl RESULT_VARIABLE INSTALL_RESULT)
    endif ()
    find_package(CURL QUIET)
    if (NOT CURL_FOUND)
        message(FATAL_ERROR "Failed to install or locate libcurl (install result=${INSTALL_RESULT})")
    endif ()
endif ()


FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 GIT_SHALLOW TRUE EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(json)


## LIBRARIES ##
set(SOURCE_OBJECTS include/sc.h)

set(OBJECT_NAME sc-utf8)
add_library(${OBJECT_NAME} OBJECT src/utf8.cpp include/utf8.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-base64)
add_library(${OBJECT_NAME} OBJECT src/base64.cpp include/base64.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-timer)
add_library(${OBJECT_NAME} OBJECT src/timer.cpp include/timer.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-color)
add_library(${OBJECT_NAME} OBJECT src/color.cpp include/color.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-rect)
add_library(${OBJECT_NAME} OBJECT src/rect.cpp include/rect.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-ollama)
add_library(${OBJECT_NAME} OBJECT src/ollama.cpp include/ollama.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${OBJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-date)
add_library(${OBJECT_NAME} OBJECT src/date.cpp include/date.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-percent)
add_library(${OBJECT_NAME} OBJECT src/percent.cpp include/percent.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

set(OBJECT_NAME sc-core)
add_library(${OBJECT_NAME} OBJECT src/core.cpp include/core.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${OBJECT_NAME}>)

## Libraries with Dependencies...
set(SOURCE_LIBRARIES)

set(OBJECT_NAME sc-rest)
add_library(${OBJECT_NAME} STATIC src/rest.cpp include/rest.h)
set_target_properties(${OBJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${OBJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${OBJECT_NAME} PUBLIC curl)
list(APPEND SOURCE_LIBRARIES ${OBJECT_NAME})

# Combined static libary for easy usage
project(sc VERSION ${SC_VERSION} DESCRIPTION "simply-cpp consolidated static library")
add_library(${PROJECT_NAME} STATIC ${SOURCE_OBJECTS})
add_library(sc::sc ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${PROJECT_NAME} PUBLIC ${SOURCE_LIBRARIES})

# Combined shared libary for easy usage
project(sc-shared VERSION ${SC_VERSION} DESCRIPTION "simply-cpp common shared library")
add_library(${PROJECT_NAME} SHARED ${SOURCE_OBJECTS})
add_library(sc::sc-shared ALIAS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(${PROJECT_NAME} PUBLIC ${SOURCE_LIBRARIES})

list(APPEND SOURCE_LIBRARIES sc)
list(APPEND SOURCE_LIBRARIES sc-shared)

install(TARGETS ${SOURCE_LIBRARIES} EXPORT scTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT scTargets FILE scTargets.cmake NAMESPACE sc:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sc)
configure_package_config_file(cmake/scConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/scConfig.cmake INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sc)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/scConfigVersion.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/scConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/scConfigVersion.cmake DESTINATION lib/cmake/sc)


if (SC_BUILD_TESTS)
    add_subdirectory(tests sc-test EXCLUDE_FROM_ALL)
endif ()