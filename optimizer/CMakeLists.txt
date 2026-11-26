
add_library(optimizer STATIC
    src/constant_evaluator.cpp
)

target_include_directories(optimizer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(optimizer PUBLIC
    parser
)

target_compile_features(optimizer PUBLIC cxx_std_23)
