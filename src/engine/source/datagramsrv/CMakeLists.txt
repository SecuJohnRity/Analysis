set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})
set(INC_DIR ${CMAKE_CURRENT_LIST_DIR}) # Assuming .hpp is in the same directory

add_library(datagramsrv STATIC
    ${SRC_DIR}/datagram_server.cpp
)

target_include_directories(datagramsrv
    PUBLIC
        ${INC_DIR} # Makes datagram_server.hpp available
    PRIVATE
        ${SRC_DIR}
)

target_link_libraries(datagramsrv
    PUBLIC # Or PRIVATE, depending on whether other modules will directly use datagramsrv types
        httpsrv
        parser
        router
        base
        # spdlog::spdlog # If using spdlog directly for logging
)

# Tests (optional, but good practice)
# if(ENGINE_BUILD_TEST)
# set(TEST_SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/test/src)
# add_executable(datagramsrv_utest
#     ${TEST_SRC_DIR}/datagram_server_test.cpp
# )
# target_include_directories(datagramsrv_utest
#     PRIVATE
#     ${TEST_SRC_DIR}
# )
# target_link_libraries(datagramsrv_utest
#     PRIVATE
#     datagramsrv
#     httpsrv # For mocking DatagramSocket if needed
#     parser  # For mocking LegacyProtocolParser if needed
#     router  # For mocking Orchestrator if needed
#     base
#     GTest::gtest_main
# )
# gtest_discover_tests(datagramsrv_utest)
# endif()
