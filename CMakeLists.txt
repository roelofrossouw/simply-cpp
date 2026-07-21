if (APPLE)
    cmake_minimum_required(VERSION 3.30)
else ()
    cmake_minimum_required(VERSION 3.10)
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(SC_VERSION 1.0.2)

# Settings
option(ADD_POSTGRESQL "Add the postgresql db module" OFF)

set(SC_SETUP_VERSION_FILE "${CMAKE_BINARY_DIR}/setup_sc.version")
if (NOT EXISTS "${SC_SETUP_VERSION_FILE}")
    execute_process(COMMAND git init -b main WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git remote add base64 https://github.com/tobiaslocker/base64.git WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git remote add lunasvg https://github.com/sammycage/lunasvg.git WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git subtree add --prefix=third_party/base64 base64 master --squash WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git subtree add --prefix=third_party/lunasvg lunasvg master --squash WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    execute_process(COMMAND git submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    if (APPLE)
        execute_process(COMMAND brew install nlohmann-json curl postgres WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    else ()
        execute_process(COMMAND apt -y install nlohmann-json3-dev libcurlpp-dev postgresql-server-dev-16 WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif ()
    file(WRITE "${SC_SETUP_VERSION_FILE}" "1")
endif ()

set(BASE64_ENABLE_TESTING OFF)
add_subdirectory(third_party/base64 base64 EXCLUDE_FROM_ALL)

set(LUNASVG_BUILD_STATIC ON)
set(LUNASVG_BUILD_EXAMPLES OFF)
add_subdirectory(third_party/lunasvg lunasvg EXCLUDE_FROM_ALL)

## Postgres ##
#execute_process(COMMAND pg_config --includedir OUTPUT_VARIABLE PG_HEADERS OUTPUT_STRIP_TRAILING_WHITESPACE)
#execute_process(COMMAND pg_config --libdir OUTPUT_VARIABLE PG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

if (${ADD_POSTGRESQL})
    if (APPLE)
        set(PostgreSQL_ROOT "/opt/homebrew/opt/postgresql/lib/postgresql")
        set(PostgreSQL_INCLUDE_DIR "/opt/homebrew/opt/postgresql/include")
        include_directories("/opt/homebrew/opt/postgresql/include/postgresql")
    endif ()
    find_package(PostgreSQL REQUIRED)
endif ()

## LIBRARIES ##
project(sc-base64)
add_library(${PROJECT_NAME} STATIC src/base64.cpp include/base64.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} base64)

project(sc-svg2png)
add_library(${PROJECT_NAME} STATIC src/svg2png.cpp include/svg2png.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC lunasvg)

project(sc-rest)
add_library(${PROJECT_NAME} STATIC src/rest.cpp include/rest.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} PUBLIC curl)

project(sc-timer)
add_library(${PROJECT_NAME} STATIC src/timer.cpp include/timer.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-color)
add_library(${PROJECT_NAME} STATIC src/color.cpp include/color.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-rect)
add_library(${PROJECT_NAME} STATIC src/rect.cpp include/rect.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-ollama)
add_library(${PROJECT_NAME} STATIC src/ollama.cpp include/ollama.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

project(sc-date)
add_library(${PROJECT_NAME} STATIC src/date.cpp include/date.h)
set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")

if (${ADD_POSTGRESQL})
    project(sc-postgres)
    add_library(${PROJECT_NAME} STATIC src/postgres.cpp include/postgres.h)
    set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL ON)
    target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR} "${PROJECT_SOURCE_DIR}/include")
    target_include_directories(${PROJECT_NAME} PUBLIC ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
    target_link_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARY_DIRS})
endif ()


set(SOURCE_OBJECTS
        src/sc.cpp sc.h
        $<TARGET_OBJECTS:sc-date>
        $<TARGET_OBJECTS:sc-base64>
        $<TARGET_OBJECTS:sc-svg2png>
        $<TARGET_OBJECTS:sc-rest>
        $<TARGET_OBJECTS:sc-timer>
        $<TARGET_OBJECTS:sc-color>
        $<TARGET_OBJECTS:sc-rect>
        $<TARGET_OBJECTS:sc-ollama>
)

set(USED_LIBRARIES
        base64
        lunasvg
        curl
)

# Combined static libary for easy usage
project(sc VERSION ${SC_VERSION} DESCRIPTION "simply-cpp common static library")
add_library(${PROJECT_NAME} STATIC ${SOURCE_OBJECTS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${USED_LIBRARIES})
if (${ADD_POSTGRESQL})
    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
endif ()
target_link_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARY_DIRS})

# Combined shared libary for easy usage
project(sc-shared VERSION ${SC_VERSION} DESCRIPTION "simply-cpp common shared library")
add_library(${PROJECT_NAME} SHARED ${SOURCE_OBJECTS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${USED_LIBRARIES})
if (${ADD_POSTGRESQL})
    target_link_libraries(${PROJECT_NAME} PUBLIC PostgreSQL::PostgreSQL)
endif ()
target_link_directories(${PROJECT_NAME} PRIVATE ${PostgreSQL_LIBRARY_DIRS})

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

project(test_postgres)
add_executable(${PROJECT_NAME} test/postgres.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_dual)
add_executable(${PROJECT_NAME} test/dual.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)

project(test_ollama)
add_executable(${PROJECT_NAME} test/ollama.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sc)
add_dependencies(test_ollama copy_cs_resources)
