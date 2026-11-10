add_library(semantic_analysis STATIC
    src/analyzer.cpp
)

target_include_directories(semantic_analysis PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(semantic_analysis PUBLIC cxx_std_23)
