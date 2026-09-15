cmake_minimum_required(VERSION 3.22)
project(simply-cpp LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

enable_testing()

# Resources for Tests #
set(RESOURCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RESOURCE_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")
add_custom_target(copy_cs_resources ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_SOURCE_DIR} ${RESOURCE_DESTINATION_DIR})

# Function to add tests...
function(add_sc_test name)
    add_executable("test-${name}" "${name}.cpp")
    target_link_libraries("test-${name}" PRIVATE sc)
    add_test(NAME "test-${name}" COMMAND "test-${name}")
endfunction()

## TESTS ##
add_sc_test(date)
add_sc_test(base64)
add_sc_test(rest)
add_sc_test(timer)
add_sc_test(color)
add_sc_test(rect)
add_sc_test(point)
add_sc_test(dual)
add_sc_test(percent)
add_sc_test(ollama)
add_dependencies(test-ollama copy_cs_resources)


