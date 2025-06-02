# CMakeLists.txt for C++ integration tests

if(ENGINE_BUILD_TEST)

# DatagramServer Integration Test
add_executable(datagram_server_it
    datagram_server_it.cpp
)

target_link_libraries(datagram_server_it
    PRIVATE
    # Engine components required by the test:
    datagramsrv  # The server itself
    httpsrv      # For DatagramSocket
    parser       # For LegacyProtocolParser
    router       # For Orchestrator type (even if mocked/stubbed)
    base         # For base::Event, json::Json, etc.

    # Google Test libraries:
    GTest::gtest      # For basic TEST_F, ASSERT_ macros
    GTest::gtest_main # For main() function for the test executable
    GTest::gmock      # If using GMock for mocking (like MockEventPoster)
)

# Discover tests from the executable
include(GoogleTest)
gtest_discover_tests(datagram_server_it)

# Add other C++ integration tests here if needed in the future

endif()
