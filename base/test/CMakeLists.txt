project(test_base)

set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

find_package(GTest)
enable_testing()

add_executable(test_base
    main.cc
    #test_subject.cc
    #10_array.cc
    #20_arraystore.cc
)

target_link_libraries(test_base
    PRIVATE
        base
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_base)