cmake_minimum_required(VERSION 3.10)

project(InventoryHarvesterTests CXX)

# Find GTest and GMock
# This might be handled by a higher-level CMake file in a real Wazuh setup.
# If not, uncomment and adjust:
# find_package(GTest REQUIRED)
# find_package(GMock REQUIRED) # GMock is often found via GTest

# Add include directories
# This should point to the root of the wazuh project to allow includes like
# "wazuh_modules/inventory_harvester/src/..."
# And also the directory containing gtest headers.
# These paths are examples and depend on the actual build environment structure.
include_directories(
    ${CMAKE_SOURCE_DIR}/src # Assuming this CMakeLists is in src/wazuh_modules/inventory_harvester/tests/unit
                            # and CMAKE_SOURCE_DIR is the root of the wazuh project.
                            # Adjust relative path if CMAKE_SOURCE_DIR is different.
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src # To access harvester sources from tests/unit
    # Add other necessary include paths, e.g., for gtest
    # ${GTEST_INCLUDE_DIRS}
)

# --- Define a library for the main module's sources ---
# This is a common practice. If UserElement/GroupElement have .cpp files, or Facade has them,
# they should be compiled into a library that tests can link against.
# Adjust source paths as needed.
# If elements are header-only, they don't need to be in this library for their own tests,
# but the Facade test would still need the Facade's compiled code.

# Conceptual: Assuming InventoryHarvesterFacade.cpp is in ../../src
# and elements are in ../../src/systemInventory/elements/
# If UserElement.cpp or GroupElement.cpp exist, add them here.
# For now, assuming UserElement.hpp and GroupElement.hpp are used and their dependencies are met by includes.
# InventoryHarvesterFacade.cpp definitely needs to be compiled.

add_library(inventory_harvester_lib_obj OBJECT
    ../../src/inventoryHarvesterFacade.cpp
    # Add other .cpp files from the main module if they are needed by the facade or elements
    # e.g., ../../src/systemInventory/elements/userElement.cpp (if it existed)
)

# --- Test for UserElement ---
add_executable(inventory_harvester_userElement_test userElement_test.cpp)
target_link_libraries(inventory_harvester_userElement_test
    PRIVATE
        gtest_main # Includes gtest and links main
        gmock      # Google Mock
        # wazuh_common # Assuming a target for common utilities (Utils::Json etc.)
                     # This might be named differently or provided by a higher CMakeLists.
                     # If Utils::Json is header only and included, direct link might not be needed.
    PRIVATE
        $<TARGET_OBJECTS:inventory_harvester_lib_obj> # Link facade if needed by elements (unlikely for elements)
                                                   # More likely for common utils if not in wazuh_common
)
add_test(NAME InventoryHarvesterUserElementTest COMMAND inventory_harvester_userElement_test)

# --- Test for GroupElement ---
add_executable(inventory_harvester_groupElement_test groupElement_test.cpp)
target_link_libraries(inventory_harvester_groupElement_test
    PRIVATE
        gtest_main
        gmock
        # wazuh_common
    PRIVATE
        $<TARGET_OBJECTS:inventory_harvester_lib_obj> # As above
)
add_test(NAME InventoryHarvesterGroupElementTest COMMAND inventory_harvester_groupElement_test)

# --- Test for InventoryHarvesterFacade ---
add_executable(inventory_harvester_facade_test inventoryHarvesterFacade_test.cpp)
target_link_libraries(inventory_harvester_facade_test
    PRIVATE
        gtest_main
        gmock
        # wazuh_common
        # flatbuffers # Assuming a target "flatbuffers" if used and not header-only
    PRIVATE
        $<TARGET_OBJECTS:inventory_harvester_lib_obj> # Link against the facade's compiled code
)
add_test(NAME InventoryHarvesterFacadeTest COMMAND inventory_harvester_facade_test)

# Enable testing if not already enabled by a higher-level CMake file
enable_testing()
