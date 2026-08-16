# Add Google C++ Testing Framework as part of this project
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

# set(TEST "T-placeholder")
# add_executable(${TEST} placeholder.cpp)
# target_link_libraries(${TEST} PRIVATE ${PROJECT_NAME} GTest::gtest_main)
# target_include_directories(${TEST} PRIVATE ${CMAKE_SOURCE_DIR}/src/${PROJECT_NAME})
# gtest_discover_tests(${TEST})
