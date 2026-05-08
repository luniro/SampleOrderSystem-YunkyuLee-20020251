add_library(persistence STATIC
    data_store.cpp
)

target_include_directories(persistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(persistence PUBLIC json_lib)

if(MSVC)
    target_compile_options(persistence PRIVATE /W4 /utf-8)
else()
    target_compile_options(persistence PRIVATE -Wall -Wextra)
endif()
