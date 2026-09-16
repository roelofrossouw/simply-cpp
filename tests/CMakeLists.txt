# Resources for Tests #
set(RESOURCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RESOURCE_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")
add_custom_target(copy_cs_resources ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_SOURCE_DIR} ${RESOURCE_DESTINATION_DIR})

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

