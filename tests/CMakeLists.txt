add_executable(test_data_store
    persistence/test_data_store.cpp
)

target_include_directories(test_data_store PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
)

target_link_libraries(test_data_store PRIVATE
    persistence
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_data_store PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_data_store PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(test_data_store)
