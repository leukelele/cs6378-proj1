# recursively gather all test source files in tests/ subdirectories
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp")

# keep track of individual test executables
set(TEST_EXECUTABLES "")

# loop through each test source and create its own executable
foreach(test_source ${TEST_SOURCES})
    get_filename_component(test_name ${test_source} NAME_WE)

    add_executable(${test_name} ${test_source} ${LIB_SOURCES})

    # include headers
    target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)

    # enable test macros
    target_compile_definitions(${test_name} PRIVATE ENABLE_TESTS)

    # link sctp library
    target_link_libraries(${test_name} PRIVATE sctp)

    # Register test with CTest
    add_test(NAME ${test_name} COMMAND ${test_name})

    list(APPEND TEST_EXECUTABLES ${test_name})
endforeach()

# optional: collective target so you can build all tests at once
add_custom_target(tests ALL DEPENDS ${TEST_EXECUTABLES})
