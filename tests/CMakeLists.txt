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

# ── Phase 0-2: dummy_generator tests ────────────────────────────────────────
add_executable(test_dummy_generator
    dummy_generator/test_dummy_generator.cpp
)

target_include_directories(test_dummy_generator PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_dummy_generator PRIVATE
    dummy_generator
    persistence
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_dummy_generator PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_dummy_generator PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_dummy_generator)

# ── Phase 0-3: monitor domain types tests ───────────────────────────────────
add_executable(test_monitor_types
    monitor/test_monitor_types.cpp
)

target_include_directories(test_monitor_types PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_monitor_types PRIVATE
    monitor_lib
    persistence
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_monitor_types PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_monitor_types PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_monitor_types)
