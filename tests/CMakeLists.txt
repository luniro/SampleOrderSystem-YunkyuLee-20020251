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

# ── Phase 0-4: mvc App menu shell tests ─────────────────────────────────────
add_executable(test_mvc_app
    mvc/test_mvc_app.cpp
)

target_include_directories(test_mvc_app PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_mvc_app PRIVATE
    mvc
    monitor_lib
    persistence
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_mvc_app PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_mvc_app PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_mvc_app)

# ── Phase 1: Timestamp + ProductionCalc utils tests ─────────────────────────
add_executable(test_phase1_utils
    monitor/test_phase1_utils.cpp
)

target_include_directories(test_phase1_utils PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/monitor
)

target_link_libraries(test_phase1_utils PRIVATE
    monitor_lib
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_phase1_utils PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_phase1_utils PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_phase1_utils)

# ── Phase 1: TablePrinter paging tests ──────────────────────────────────────
add_executable(test_table_paging
    monitor/test_table_paging.cpp
)

target_include_directories(test_table_paging PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/monitor
)

target_link_libraries(test_table_paging PRIVATE
    monitor_lib
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_table_paging PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_table_paging PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_table_paging)

# ── Phase 1: InputUtil tests ─────────────────────────────────────────────────
add_executable(test_input_util
    mvc/test_input_util.cpp
)

# ── Phase 2: Sample Management tests ─────────────────────────────────────────
add_executable(test_sample_management
    mvc/test_sample_management.cpp
)

target_include_directories(test_input_util PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_input_util PRIVATE
    mvc
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_input_util PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_input_util PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_input_util)

target_include_directories(test_sample_management PRIVATE
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/lib/persistence
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_sample_management PRIVATE
    mvc
    monitor_lib
    persistence
    GTest::gtest_main
)

if(MSVC)
    target_compile_options(test_sample_management PRIVATE /W4 /utf-8)
else()
    target_compile_options(test_sample_management PRIVATE -Wall -Wextra)
endif()

gtest_discover_tests(test_sample_management)
