set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}) # Assuming .hpp is in the same directory

# Library for the parser module
add_library(parser STATIC
    ${SRC_DIR}/legacy_protocol_parser.cpp
)

target_include_directories(parser
    PUBLIC
        ${INC_DIR} # Makes legacy_protocol_parser.hpp available to others
    PRIVATE
        ${SRC_DIR}
)

# Add dependencies if any, for example:
# target_link_libraries(parser
#     PUBLIC
#         base # If it uses something from the 'base' library
# )

# Tests (optional, but good practice)
# if(ENGINE_BUILD_TEST)
# set(TEST_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/test/src)
# add_executable(parser_utest
#     ${TEST_SRC_DIR}/legacy_protocol_parser_test.cpp
# )
# target_include_directories(parser_utest
#     PRIVATE
#     ${TEST_SRC_DIR}
# )
# target_link_libraries(parser_utest
#     PRIVATE
#     parser
#     GTest::gtest_main
# )
# gtest_discover_tests(parser_utest)
# endif()
