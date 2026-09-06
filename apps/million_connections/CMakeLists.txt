set(EXECUTABLE "mcon_client")
add_executable(${EXECUTABLE} mcon_client.c)
target_link_libraries(${EXECUTABLE} ${PROJECT_NAME})
set_target_properties(${EXECUTABLE}
    PROPERTIES 
    LINKER_LANGUAGE C
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/../bin
)
