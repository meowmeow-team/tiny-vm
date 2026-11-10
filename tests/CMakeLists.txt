add_executable(unit_tests
    test_main.cpp
    test_core.cpp
    test_lexer.cpp
    test_parser.cpp
    ../core/logger.cpp
    ../core/exception.cpp
    ../core/source_manager.cpp
)

target_include_directories(unit_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/lexer/include
    ${CMAKE_SOURCE_DIR}/parser/include
)

target_link_libraries(unit_tests PRIVATE 
    Catch2::Catch2WithMain
    lexer
    parser
)

include(Catch)
catch_discover_tests(unit_tests)