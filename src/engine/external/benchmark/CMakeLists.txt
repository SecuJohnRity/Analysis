# CMakeLists.txt for benchmark (source build)
# This file is a placeholder and might need adjustments based on
# the actual CMake structure of the benchmark library.
# It's typically included via add_subdirectory() from WazuhExternalDependencies.cmake

cmake_minimum_required(VERSION 3.22.1)

project(wazuh_ext_benchmark NONE)

message(STATUS "Building benchmark from source via external/benchmark/CMakeLists.txt (placeholder)")

# This is a guess. The actual benchmark CMake might be more complex.
# We are in ${WAZUH_EXTERN_DIR}/benchmark-1.8.5-src,
# and its CMakeLists.txt is at the root of its source tree.
# The add_subdirectory in WazuhExternalDependencies.cmake already points to this source dir.
# So, this file might not even be needed if benchmark's own CMakeLists.txt is sufficient
# and correctly picked up by the add_subdirectory command in WazuhExternalDependencies.cmake.

# However, the `add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR})` in WazuhExternalDependencies.cmake
# implies that benchmark's CMakeLists.txt is directly used.
# This file (src/engine/external/benchmark/CMakeLists.txt) might be redundant if that's the case.
# For now, let's assume it's NOT needed and the main add_subdirectory in WazuhExternalDependencies.cmake
# handles it. If benchmark requires a custom wrapper CMakeLists.txt, this would be the place.

# If benchmark's CMakeLists.txt needs specific variables set, do it *before* the
# add_subdirectory call in WazuhExternalDependencies.cmake.
# Example:
# set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
# set(BENCHMARK_ENABLE_INSTALL ON CACHE BOOL "Enable benchmark install" FORCE)
# add_subdirectory(\${DEP_SOURCE_DIR} \${DEP_BUILD_DIR})

# For now, this file will be empty as google/benchmark has its own CMakeLists.txt.
# The `add_subdirectory` in `WazuhExternalDependencies.cmake` should point to the
# *actual* source directory of benchmark, not this `src/engine/external/benchmark` directory.

# Correcting the plan: The `add_subdirectory` in `wazuh_add_dependency_benchmark`
# should directly use the downloaded source: `add_subdirectory(\${DEP_SOURCE_DIR} \${DEP_BUILD_DIR})`.
# This means we don't need a *separate* `src/engine/external/benchmark/CMakeLists.txt` to *wrap* benchmark's build.
# The build system of benchmark itself will be used.

# The subtask should ensure the directory `src/engine/external/` exists as a common place for external sources,
# but this specific CMakeLists.txt (`src/engine/external/benchmark/CMakeLists.txt`) is not strictly necessary
# if benchmark's own CMake system is invoked directly.
# Let's keep this file empty for now to signify that benchmark's own CMake is used.
message(STATUS "This CMakeLists.txt (src/engine/external/benchmark/CMakeLists.txt) is a placeholder. Benchmark's own CMake script is used directly.")
