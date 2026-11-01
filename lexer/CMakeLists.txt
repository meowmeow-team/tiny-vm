add_library(lexer STATIC
    src/lexer.cpp
    src/token.cpp
    src/type_holder.cpp
)

target_include_directories(lexer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(lexer PUBLIC cxx_std_23)
