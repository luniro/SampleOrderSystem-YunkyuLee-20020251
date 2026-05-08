add_library(dummy_generator STATIC
    dummy_generator.cpp
)

target_include_directories(dummy_generator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(dummy_generator PRIVATE persistence)

if(MSVC)
    target_compile_options(dummy_generator PRIVATE /W4 /utf-8)
else()
    target_compile_options(dummy_generator PRIVATE -Wall -Wextra)
endif()
