add_library(codegen STATIC
)

target_include_directories(codegen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(codegen PUBLIC cxx_std_23)
