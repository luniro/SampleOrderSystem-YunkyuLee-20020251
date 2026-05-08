add_library(monitor_lib STATIC
    domain/types.cpp
    repository/sample_repository.cpp
    repository/order_repository.cpp
    repository/production_repository.cpp
    ui/table_printer.cpp
    ui/console.cpp
    ui/app.cpp
    util/timestamp.cpp
    util/production_calc.cpp
)

target_include_directories(monitor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monitor_lib PUBLIC persistence)

if(MSVC)
    target_compile_options(monitor_lib PRIVATE /W4 /utf-8)
else()
    target_compile_options(monitor_lib PRIVATE -Wall -Wextra)
endif()
