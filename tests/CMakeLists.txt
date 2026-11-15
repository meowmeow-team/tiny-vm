add_executable(unit_tests
    test_main.cpp
    test_core.cpp
    test_lexer.cpp
    test_parser.cpp
    test_semantic_analysis.cpp
    test_semantic_randomized.cpp
    ../core/logger.cpp
    ../core/exception.cpp
    ../core/source_manager.cpp
)

target_include_directories(unit_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/lexer/include
    ${CMAKE_SOURCE_DIR}/parser/include
    ${CMAKE_SOURCE_DIR}/semantic_analysis/include
)

target_link_libraries(unit_tests PRIVATE 
    Catch2::Catch2WithMain
    lexer
    parser
    semantic_analysis
)

include(Catch)
catch_discover_tests(unit_tests)
