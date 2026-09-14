cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project(simply-cpp)

#execute_process(
#        COMMAND git describe --tags --abbrev=0
#        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#        OUTPUT_VARIABLE PROJECT_VERSION
#        OUTPUT_STRIP_TRAILING_WHITESPACE
#)
#string(REGEX REPLACE "^v" "" PROJECT_VERSION "${PROJECT_VERSION}")
set(SC_VERSION 1.0.8)
#
#message(STATUS "Project Version: ${PROJECT_VERSION}")
#message(STATUS "SC Version: ${SC_VERSION}")


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
endif ()

if (NOT CURL_FOUND)
    message(FATAL_ERROR "Failed to install or locate libcurl (apt result=${INSTALL_RESULT})")
endif ()

#    if (APPLE)
#    else ()
#        execute_process(COMMAND apt -y install libcurlpp-dev WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#    endif ()
#    file(WRITE "${SC_SETUP_VERSION_FILE}" "1")
#endif ()


include(FetchContent)
FetchContent_Declare(
        lunasvg
        GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
        GIT_TAG origin/master
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
set(LUNASVG_BUILD_STATIC ON)
set(LUNASVG_BUILD_EXAMPLES OFF)
FetchContent_MakeAvailable(lunasvg)

FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(json)


## Postgres ##
#    if (APPLE)
#        execute_process(COMMAND brew install postgres WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#    else ()
#        execute_process(COMMAND apt -y install postgresql-server-dev-16 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
#    endif ()

#execute_process(COMMAND pg_config --includedir OUTPUT_VARIABLE PG_HEADERS OUTPUT_STRIP_TRAILING_WHITESPACE)
#execute_process(COMMAND pg_config --libdir OUTPUT_VARIABLE PG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
#if (${ADD_POSTGRESQL})
#    if (APPLE)
#        set(PostgreSQL_ROOT "/opt/homebrew/opt/postgresql/lib/postgresql")
#        set(PostgreSQL_INCLUDE_DIR "/opt/homebrew/opt/postgresql/include")
#        include_directories("/opt/homebrew/opt/postgresql/include/postgresql")
#    endif ()
#    find_package(PostgreSQL REQUIRED)
#endif ()

## LIBRARIES ##
project(sc-utf8)
add_library(${PROJECT_NAME} OBJECT src/utf8.cpp include/utf8.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")

project(sc-base64)
add_library(${PROJECT_NAME} OBJECT src/base64.cpp include/base64.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")

project(sc-svg2png)
add_library(${PROJECT_NAME} OBJECT src/svg2png.cpp include/svg2png.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC lunasvg)

project(sc-rest)
add_library(${PROJECT_NAME} OBJECT src/rest.cpp include/rest.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-timer)
add_library(${PROJECT_NAME} OBJECT src/timer.cpp include/timer.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-color)
add_library(${PROJECT_NAME} OBJECT src/color.cpp include/color.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-rect)
add_library(${PROJECT_NAME} OBJECT src/rect.cpp include/rect.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-ollama)
add_library(${PROJECT_NAME} OBJECT src/ollama.cpp include/ollama.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)

project(sc-date)
add_library(${PROJECT_NAME} OBJECT src/date.cpp include/date.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-percent)
add_library(${PROJECT_NAME} OBJECT src/percent.cpp include/percent.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-core)
add_library(${PROJECT_NAME} OBJECT src/sc.cpp sc.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

#if (${ADD_POSTGRESQL})
#    project(sc-postgres)
#    add_library(${PROJECT_NAME} OBJECT src/postgres.cpp include/postgres.h)
#    set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
#    target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
#    target_include_directories(${PROJECT_NAME} PUBLIC ${PostgreSQL_INCLUDE_DIRS})
#    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
#    target_link_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARY_DIRS})
#endif ()


set(SOURCE_OBJECTS
        $<TARGET_OBJECTS:sc-date>
        $<TARGET_OBJECTS:sc-percent>
        $<TARGET_OBJECTS:sc-utf8>
        $<TARGET_OBJECTS:sc-base64>
        $<TARGET_OBJECTS:sc-svg2png>
        $<TARGET_OBJECTS:sc-rest>
        $<TARGET_OBJECTS:sc-timer>
        $<TARGET_OBJECTS:sc-color>
        $<TARGET_OBJECTS:sc-rect>
        $<TARGET_OBJECTS:sc-ollama>
        $<TARGET_OBJECTS:sc-core>
)

set(SOURCE_LIBRARIES
        CURL::libcurl
)
#if (${ADD_POSTGRESQL})
#    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
#endif ()
# target_link_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARY_DIRS})


# Combined static libary for easy usage
project(sc VERSION ${SC_VERSION} DESCRIPTION "simply-cpp consolidated static library")
add_library(${PROJECT_NAME} STATIC ${SOURCE_OBJECTS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SOURCE_LIBRARIES})

# Combined shared libary for easy usage
project(sc-shared VERSION ${SC_VERSION} DESCRIPTION "simply-cpp common shared library")
add_library(${PROJECT_NAME} SHARED src/sc.cpp sc.h)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${SOURCE_LIBRARIES})

# Resources for Tests ##
set(RESOURCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/test/resource")
set(RESOURCE_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")
add_custom_target(copy_cs_resources ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_SOURCE_DIR} ${RESOURCE_DESTINATION_DIR})


## TESTS ##
project(test_date)
add_executable(${PROJECT_NAME} test/date.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_base64)
add_executable(${PROJECT_NAME} test/base64.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_rest)
add_executable(${PROJECT_NAME} test/rest.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_svg2png)
add_executable(${PROJECT_NAME} test/svg2png.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_timer)
add_executable(${PROJECT_NAME} test/timer.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_color)
add_executable(${PROJECT_NAME} test/color.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_rect)
add_executable(${PROJECT_NAME} test/rect.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_point)
add_executable(${PROJECT_NAME} test/point.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

#project(test_postgres)
#add_executable(${PROJECT_NAME} test/postgres.cpp)
#target_link_libraries(${PROJECT_NAME} PRIVATE sc sc-postgres)

project(test_dual)
add_executable(${PROJECT_NAME} test/dual.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_percent)
add_executable(${PROJECT_NAME} test/percent.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_ollama)
add_executable(${PROJECT_NAME} test/ollama.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)
add_dependencies(test_ollama copy_cs_resources)
