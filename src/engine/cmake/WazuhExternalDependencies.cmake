# WazuhExternalDependencies.cmake
#
# This module handles the downloading and building of external dependencies
# for the Wazuh engine.
#
# It supports two modes, controlled by the EXTERNAL_SRC_ONLY environment variable:
# - EXTERNAL_SRC_ONLY=0 (default): Downloads pre-compiled binaries from S3.
# - EXTERNAL_SRC_ONLY=1: Downloads source code and builds locally.

cmake_minimum_required(VERSION 3.22.1)

# Variables
# Base URL for S3 pre-compiled binaries (placeholder)
set(WAZUH_S3_BASE_URL "https://s3.amazonaws.com/wazuh-precompiled-dependencies") # TODO: Replace with actual URL
# Directory to store downloaded sources and builds
set(WAZUH_EXTERN_DIR "${CMAKE_BINARY_DIR}/_deps")
# Directory to install binaries to (for binary mode)
set(WAZUH_EXTERN_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/dist")

# Environment variable check
if(DEFINED ENV{EXTERNAL_SRC_ONLY} AND "$ENV{EXTERNAL_SRC_ONLY}" STREQUAL "1")
    set(WAZUH_EXTERNAL_SRC_ONLY ON)
    message(STATUS "Dependency mode: Building from source (EXTERNAL_SRC_ONLY=1)")
else()
    set(WAZUH_EXTERNAL_SRC_ONLY OFF)
    message(STATUS "Dependency mode: Using pre-compiled binaries (EXTERNAL_SRC_ONLY=0)")
endif()

# Helper function to download and extract
# Usage: wazuh_download_and_extract(
#           NAME <name>
#           VERSION <version>
#           URL <source_url_or_s3_binary_url>
#           SHA256 <expected_sha256_hash_optional>
#           EXTRACT_DIR <where_to_extract_sources_or_binaries>
#           PATCH_COMMAND <command_to_run_after_extraction_optional>
#           PATCH_WORKING_DIRECTORY <working_dir_for_patch_command_optional>
# )
function(wazuh_download_and_extract)
    set(options )
    set(oneValueArgs NAME VERSION URL SHA256 EXTRACT_DIR PATCH_COMMAND PATCH_WORKING_DIRECTORY)
    set(multiValueArgs )
    cmake_parse_arguments(DL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT DEFINED DL_NAME OR NOT DEFINED DL_VERSION OR NOT DEFINED DL_URL OR NOT DEFINED DL_EXTRACT_DIR)
        message(FATAL_ERROR "wazuh_download_and_extract: NAME, VERSION, URL, and EXTRACT_DIR are required.")
    endif()

    # Ensure wget and tar are available
    find_program(WGET_EXECUTABLE wget REQUIRED)
    find_program(TAR_EXECUTABLE tar REQUIRED)

    set(ARCHIVE_DEST "${WAZUH_EXTERN_DIR}/${DL_NAME}-${DL_VERSION}.tar.gz")

    # Create download and extraction directories
    file(MAKE_DIRECTORY "${WAZUH_EXTERN_DIR}")
    file(MAKE_DIRECTORY "${DL_EXTRACT_DIR}")

    # Check if already downloaded and extracted (simple check, could be more robust)
    # For source builds, we check if the EXTRACT_DIR is non-empty.
    # For binary installs, a specific file/flag could be checked.
    if(EXISTS "${ARCHIVE_DEST}" AND ( (WAZUH_EXTERNAL_SRC_ONLY AND IS_DIRECTORY "${DL_EXTRACT_DIR}" AND NOT IS_EMPTY "${DL_EXTRACT_DIR}") OR (NOT WAZUH_EXTERNAL_SRC_ONLY AND EXISTS "${WAZUH_EXTERN_INSTALL_DIR}/${DL_NAME}/.installed")) )
        message(STATUS "Dependency ${DL_NAME} version ${DL_VERSION} already processed.")
        return()
    endif()

    message(STATUS "Downloading ${DL_NAME} version ${DL_VERSION} from ${DL_URL} to ${ARCHIVE_DEST}")
    execute_process(
        COMMAND "${WGET_EXECUTABLE}" "${DL_URL}" -O "${ARCHIVE_DEST}"
        RESULT_VARIABLE WGET_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT WGET_RESULT EQUAL 0)
        file(REMOVE "${ARCHIVE_DEST}")
        message(FATAL_ERROR "Failed to download ${DL_NAME} from ${DL_URL}. Error code: ${WGET_RESULT}")
    endif()

    if(DEFINED DL_SHA256 AND NOT DL_SHA256 STREQUAL "")
        message(STATUS "Verifying SHA256 for ${ARCHIVE_DEST}")
        file(SHA256 "${ARCHIVE_DEST}" ARCHIVE_SHA256)
        if(NOT ARCHIVE_SHA256 STREQUAL DL_SHA256)
            file(REMOVE "${ARCHIVE_DEST}")
            message(FATAL_ERROR "SHA256 mismatch for ${ARCHIVE_DEST}. Expected ${DL_SHA256}, got ${ARCHIVE_SHA256}.")
        endif()
    endif()

    message(STATUS "Extracting ${ARCHIVE_DEST} to ${DL_EXTRACT_DIR}")
    execute_process(
        COMMAND "${TAR_EXECUTABLE}" xzf "${ARCHIVE_DEST}" --strip-components=1 -C "${DL_EXTRACT_DIR}"
        RESULT_VARIABLE TAR_RESULT
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(NOT TAR_RESULT EQUAL 0)
        file(REMOVE_RECURSE "${DL_EXTRACT_DIR}")
        message(FATAL_ERROR "Failed to extract ${ARCHIVE_DEST}. Error code: ${TAR_RESULT}")
    endif()

    # Apply patch if provided
    if(DEFINED DL_PATCH_COMMAND AND NOT "${DL_PATCH_COMMAND}" STREQUAL "")
        message(STATUS "Patching ${DL_NAME} in ${DL_EXTRACT_DIR}")
        if(NOT DEFINED DL_PATCH_WORKING_DIRECTORY OR "${DL_PATCH_WORKING_DIRECTORY}" STREQUAL "")
            set(DL_PATCH_WORKING_DIRECTORY "${DL_EXTRACT_DIR}")
        endif()

        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env "${DL_PATCH_COMMAND}"
            WORKING_DIRECTORY "${DL_PATCH_WORKING_DIRECTORY}"
            RESULT_VARIABLE PATCH_RESULT
            OUTPUT_VARIABLE PATCH_OUTPUT
            ERROR_VARIABLE PATCH_ERROR
        )
        if(NOT PATCH_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to patch ${DL_NAME}. Error: ${PATCH_ERROR} Output: ${PATCH_OUTPUT}")
        endif()
        message(STATUS "Patch applied successfully for ${DL_NAME}.")
    endif()

    # For binary mode, create a marker file
    if(NOT WAZUH_EXTERNAL_SRC_ONLY)
        file(WRITE "${WAZUH_EXTERN_INSTALL_DIR}/${DL_NAME}/.installed" "Installed")
    endif()

    message(STATUS "Successfully processed ${DL_NAME} version ${DL_VERSION}.")
endfunction()

# Placeholder for dependency-specific functions/macros
# e.g., wazuh_add_dependency_benchmark()
#       wazuh_add_dependency_curl()
#       ... etc.

# --- benchmark ---
function(wazuh_add_dependency_benchmark)
    set(DEP_NAME "benchmark")
    set(DEP_VERSION "1.8.5")
    set(DEP_SOURCE_URL "https://github.com/google/benchmark/archive/refs/tags/v${DEP_VERSION}.tar.gz") # Note the 'v' prefix for benchmark tag
    # This SHA256 is for v1.8.5.tar.gz from the URL above.
    set(DEP_SHA256 "309b53f7d91d0b17127e2c070f0d922a30d802743192b11011dfc4d20678935f")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Disable benchmark's own tests to avoid gtest conflicts and unnecessary builds.
        set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark's own unit tests" FORCE)
        # Disable installation from benchmark's CMake, we might handle it differently or not need it if linking statically.
        set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark's install rule" FORCE)
        # Prevent benchmark from trying to download dependencies (like Google Test)
        set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "Prevent benchmark from downloading its own dependencies" FORCE)

        message(STATUS "Configuring benchmark source build in ${DEP_SOURCE_DIR} (build dir: ${DEP_BUILD_DIR})")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        if(TARGET benchmark)
            add_library(wazuh::benchmark ALIAS benchmark)
            # If benchmark is built as a static library and we are using it in other shared libs or executables,
            # we might need to set PIC. Google Benchmark's CMake already tries to handle this.
            # set_target_properties(benchmark PROPERTIES POSITION_INDEPENDENT_CODE ON)
        else()
            message(FATAL_ERROR "Target 'benchmark' not found after attempting to build ${DEP_NAME}. Check its CMakeLists.txt and build process.")
        endif()

        # benchmark_main is usually linked by test executables, not by the main library.
        # We provide it as wazuh::benchmark_main for users who need it.
        if(TARGET benchmark_main)
            add_library(wazuh::benchmark_main ALIAS benchmark_main)
            # set_target_properties(benchmark_main PROPERTIES POSITION_INDEPENDENT_CODE ON)
        else()
            message(WARNING "Target 'benchmark_main' not found for ${DEP_NAME}. This might be okay if not running benchmark's own main.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package if available
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        # Define imported library target for benchmark
        # This assumes the pre-compiled package installs library and headers to standard paths
        # within DEP_INSTALL_DIR (e.g., lib/ and include/)
        set(BENCHMARK_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        # Adjust lib name and path as per actual package structure
        set(BENCHMARK_LIBRARY "${DEP_INSTALL_DIR}/lib/libbenchmark.a")  # Example for static lib
        set(BENCHMARK_MAIN_LIBRARY "${DEP_INSTALL_DIR}/lib/libbenchmark_main.a")

        if(EXISTS "${BENCHMARK_LIBRARY}")
            add_library(wazuh::benchmark STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::benchmark PROPERTIES
                IMPORTED_LOCATION "${BENCHMARK_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${BENCHMARK_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::benchmark library not found at ${BENCHMARK_LIBRARY}. S3 package might be missing or structured differently.")
        endif()

        if(EXISTS "${BENCHMARK_MAIN_LIBRARY}")
            add_library(wazuh::benchmark_main STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::benchmark_main PROPERTIES
                IMPORTED_LOCATION "${BENCHMARK_MAIN_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${BENCHMARK_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::benchmark_main library not found at ${BENCHMARK_MAIN_LIBRARY}. S3 package might be missing or structured differently.")
        endif()

    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process benchmark
wazuh_add_dependency_benchmark()

# --- concurrentqueue ---
function(wazuh_add_dependency_concurrentqueue)
    set(DEP_NAME "concurrentqueue")
    set(DEP_VERSION "1.0.4")
    set(DEP_SOURCE_URL "https://github.com/cameron314/concurrentqueue/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for concurrentqueue v1.0.4.tar.gz
    set(DEP_SHA256 "c9018531a597b6e5c137452e000710e1087aa030155f01cb746c60e8f081f111")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-headers.tar.gz")

    # For header-only, SOURCE_DIR and INSTALL_DIR might point to the same effective location for includes
    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")

    # Common include path relative to EXTRACT_DIR (assuming headers are at the root of the tarball)
    set(INCLUDE_DIR_RELATIVE_PATH ".")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        add_library(wazuh::concurrentqueue INTERFACE)
        target_include_directories(wazuh::concurrentqueue INTERFACE "${DEP_SOURCE_DIR}/${INCLUDE_DIR_RELATIVE_PATH}")
        # concurrentqueue also has a CMakeLists.txt that defines `cameron314::concurrentqueue`
        # We can try to use that if needed, but for header-only, directly setting include path is often enough.
        # If we use its CMake, it would be:
        # add_subdirectory(${DEP_SOURCE_DIR} ${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build EXCLUDE_FROM_ALL)
        # add_library(wazuh::concurrentqueue ALIAS cameron314::concurrentqueue)
        # For now, let's stick to the simpler INTERFACE target directly.

    else() # Binary mode (downloading headers from S3)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package if available
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        add_library(wazuh::concurrentqueue INTERFACE IMPORTED)
        # Assuming headers are directly in DEP_INSTALL_DIR or a subdirectory like 'include'
        # Based on typical vcpkg structure, it might be ${DEP_INSTALL_DIR}/include
        # For concurrentqueue, the headers are directly in the root after extraction.
        set(CONCURRENTQUEUE_INCLUDE_DIR "${DEP_INSTALL_DIR}/${INCLUDE_DIR_RELATIVE_PATH}")
        set_target_properties(wazuh::concurrentqueue PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CONCURRENTQUEUE_INCLUDE_DIR}")
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME} (header-only)")
endfunction()

# Call the function to process concurrentqueue
wazuh_add_dependency_concurrentqueue()

# --- curl ---
function(wazuh_add_dependency_curl)
    set(DEP_NAME "curl")
    set(DEP_VERSION "8.11.0") # Based on tarball name curl-8_11_0.tar.gz
    set(DEP_SOURCE_URL "https://github.com/curl/curl/archive/refs/tags/curl-${DEP_VERSION}.tar.gz") # Adjusted URL to match version
    # SHA256 for curl-8.11.0.tar.gz
    set(DEP_SHA256 "18a53560f87c80a340c20746a8bfda0105a4ef2c9500aa389fd2196ad673cb97")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # CMake options for curl
        # Ensure OpenSSL is found if wazuh::openssl is available
        set(CURL_CMAKE_ARGS
            -DBUILD_TESTING=OFF
            -DBUILD_SHARED_LIBS=OFF # Build static libs
            -DCURL_DISABLE_LDAP=ON
            -DCURL_DISABLE_LDAPS=ON
            -DCURL_USE_OPENSSL=ON
            -DOPENSSL_USE_STATIC_LIBS=ON
            # The following might be needed if OpenSSL is not found automatically
            # It assumes wazuh::openssl provides these variables, or they are set globally
            # -DOPENSSL_ROOT_DIR=${wazuh_openssl_INSTALL_DIR} # Example, if wazuh::openssl sets this
            # -DOPENSSL_INCLUDE_DIR=${wazuh_openssl_INCLUDE_DIR}
            # -DOPENSSL_LIBRARIES=${wazuh_openssl_LIBRARIES}
        )

        # If wazuh::openssl target exists and has an install directory, try to use it
        if(TARGET wazuh::openssl)
            # This assumes wazuh::openssl is an IMPORTED target or has INTERFACE_INCLUDE_DIRECTORIES
            # and INTERFACE_LINK_LIBRARIES set.
            # For source builds, OpenSSL might be built into _deps/openssl-VERSION-install
            # We need to ensure curl's CMake can find it.
            # One way is to set CMAKE_PREFIX_PATH or specific OpenSSL_DIR for find_package(OpenSSL)
            # For now, let's assume find_package(OpenSSL) will work if OpenSSL is built first.
            # We might need to pass OPENSSL_ROOT_DIR if OpenSSL is installed to a custom prefix.
            # If wazuh_openssl_INSTALL_DIR is a variable set by wazuh_add_dependency_openssl, use it.
            # This part is tricky as OpenSSL is later in the list.
            # For now, we rely on OpenSSL being findable if already processed.
            # A better approach would be to ensure OpenSSL is processed first.
            # Or, to pass down the install directory of OpenSSL if known.
            # Let's assume for now that OpenSSL (if built from source) will be in a path
            # that CMAKE_PREFIX_PATH (set by us globally to WAZUH_EXTERN_INSTALL_DIR) would cover.
            list(APPEND CURL_CMAKE_ARGS -DCMAKE_PREFIX_PATH="${WAZUH_EXTERN_INSTALL_DIR}")
        endif()

        message(STATUS "Configuring curl source build with args: ${CURL_CMAKE_ARGS}")
        # When add_subdirectory is called, it creates a new scope.
        # Variables set with set(VAR VALUE) are local to the current scope (WazuhExternalDependencies.cmake)
        # or the new scope (curl's CMakeLists.txt).
        # To pass CURL_CMAKE_ARGS to curl's CMake, they must be CACHE variables or set before add_subdirectory
        # in a way that curl's CMakeLists.txt will see them.
        # A common way is to pass them via configure_file or by setting them as CACHE args.
        # However, add_subdirectory itself doesn't directly take a list of CMake args.
        # The variables need to be set in the PARENT_SCOPE or as CACHE variables *before* add_subdirectory
        # if curl's CMakeLists.txt expects them as simple variables.
        # For options like BUILD_TESTING, these are typically cache variables in the target project.
        # So, setting them with "CACHE BOOL" should work.
        # Let's redefine how CURL_CMAKE_ARGS are applied. We set them as cache variables before add_subdirectory.

        set(BUILD_TESTING OFF CACHE BOOL "Disable curl tests" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build curl static libraries" FORCE)
        set(CURL_DISABLE_LDAP ON CACHE BOOL "Disable LDAP in curl" FORCE)
        set(CURL_DISABLE_LDAPS ON CACHE BOOL "Disable LDAPS in curl" FORCE)
        set(CURL_USE_OPENSSL ON CACHE BOOL "Use OpenSSL in curl" FORCE)
        set(OPENSSL_USE_STATIC_LIBS ON CACHE BOOL "Link OpenSSL statically with curl" FORCE)
        # If OpenSSL is installed in a custom location by our script, help curl find it.
        # This assumes OpenSSL has been processed and WAZUH_EXTERN_INSTALL_DIR/openssl exists.
        set(OPENSSL_ROOT_DIR ${WAZUH_EXTERN_INSTALL_DIR}/openssl CACHE PATH "Path to OpenSSL installation for curl" FORCE)

        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        if(TARGET libcurl)
            add_library(wazuh::curl ALIAS libcurl)
        elseif(TARGET CURL::libcurl) # Some CMake versions of Curl might provide this
            add_library(wazuh::curl ALIAS CURL::libcurl)
        else()
            message(FATAL_ERROR "Target 'libcurl' or 'CURL::libcurl' not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(CURL_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        # Adjust lib name as per actual package (e.g., libcurl.a, libcurl.lib)
        set(CURL_LIBRARY "${DEP_INSTALL_DIR}/lib/libcurl.a") # Example for static lib

        if(EXISTS "${CURL_LIBRARY}")
            add_library(wazuh::curl STATIC IMPORTED GLOBAL) # Assuming static, adjust if shared
            set_target_properties(wazuh::curl PROPERTIES
                IMPORTED_LOCATION "${CURL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}")
                # Curl might have other interface link libraries (like OpenSSL, zlib)
                # These need to be added if the precompiled binary doesn't bundle them statically.
                # INTERFACE_LINK_LIBRARIES "wazuh::openssl;wazuh::zlib" # Example
            # For Windows, .lib file might be different, and DLL needs to be handled.
            # For Linux shared, it would be .so
        else()
            message(WARNING "wazuh::curl library not found at ${CURL_LIBRARY}. S3 package might be missing or structured differently.")
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process curl
wazuh_add_dependency_curl()

# --- date (Howard Hinnant) ---
function(wazuh_add_dependency_date)
    set(DEP_NAME "date")
    set(DEP_COMMIT "1ead6715dec030d340a316c927c877a3c4e5a00c")
    set(DEP_VERSION "1ead671") # Short version from commit
    set(DEP_SOURCE_URL "https://github.com/HowardHinnant/date/archive/${DEP_COMMIT}.tar.gz")
    # SHA256 for 1ead6715dec030d340a316c927c877a3c4e5a00c.tar.gz
    set(DEP_SHA256 "56c87095988801089a8c858780907f99179755354990526a5901055a2a2ead31")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    set(PATCH_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/patches/date-howardhinnant-cmake.patch")
    # Ensure git is available for applying patch
    find_program(GIT_EXECUTABLE git)
    if(NOT GIT_EXECUTABLE)
        message(FATAL_ERROR "git is required to apply patches for the 'date' library, but git was not found.")
    endif()
    # Using -E `env` and `git apply` for the patch command.
    set(DEP_PATCH_COMMAND "${GIT_EXECUTABLE} apply --ignore-space-change --ignore-whitespace ${PATCH_FILE}")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION} # Using short commit hash as version
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
            PATCH_COMMAND "${DEP_PATCH_COMMAND}"
            PATCH_WORKING_DIRECTORY "${DEP_SOURCE_DIR}"
        )

        # CMake options for date library
        # The patch already sets AUTO_DOWNLOAD=0. HAS_REMOTE_API=1 is also set by the patch.
        # Set them as CACHE variables before add_subdirectory
        set(ENABLE_DATE_TESTING OFF CACHE BOOL "Disable date tests" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build date static libraries" FORCE)
        set(USE_SYSTEM_TZ_DB OFF CACHE BOOL "Use bundled/downloaded IANA tzdb for date lib" FORCE)
        set(INSTALL_LIB ON CACHE BOOL "Enable library installation for date lib" FORCE)
        set(INSTALL_INCLUDE ON CACHE BOOL "Enable header installation for date lib" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for date lib" FORCE)

        message(STATUS "Configuring date source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        # Explicitly add install step for the 'date' library to ensure it populates DEP_INSTALL_DIR
        # This is crucial if other libraries need to find headers/libs from 'date' via CMAKE_PREFIX_PATH.
        # The `CMAKE_INSTALL_PREFIX` set above should guide the install targets from date's CMake.
        # We add a custom command to ensure the 'install' target of the external project is invoked.
        # This requires knowing the actual build target that triggers compilation, let's assume it's the default target.
        # A more robust way would be to find all targets created by add_subdirectory and make a custom
        # target depend on them, then make the install command depend on that custom target.
        # For now, let's assume building the default target in DEP_BUILD_DIR is enough before install.

        # This custom command ensures 'install' is run after the build of date's targets.
        # It's a bit of a workaround; ideally, date's CMake would install to CMAKE_INSTALL_PREFIX
        # as part of its normal build process if configured correctly.
        # The add_custom_target approach is more reliable.

        # Create a custom target that triggers the install for 'date'
        # This ensures that the install step is executed as part of the build process for this dependency.
        if(NOT TARGET wazuh_ext_date_install_step)
            add_custom_target(wazuh_ext_date_install_step
                COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install
                COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                DEPENDS # This should depend on the actual library targets from date, e.g. date::date after add_subdirectory
                        # However, these targets are not known until after add_subdirectory.
                        # This is a classic CMake ordering problem.
                        # For now, we assume add_subdirectory populates targets, and then we can refer to them.
                        # A simpler approach is to make the ALIAS targets depend on this install step.
            )
        endif()


        # Date library creates targets like date::date and date::date-tz
        # Ensure these alias targets depend on the installation.
        if(TARGET date::date)
            add_library(wazuh::date ALIAS date::date)
            # add_dependencies(wazuh::date wazuh_ext_date_install_step) # This creates a circular dependency if date::date is needed by install
        else()
            message(FATAL_ERROR "Target 'date::date' not found after building ${DEP_NAME}.")
        endif()
        if(TARGET date::date-tz)
            add_library(wazuh::date-tz ALIAS date::date-tz)
            # add_dependencies(wazuh::date-tz wazuh_ext_date_install_step)
        else()
            message(FATAL_ERROR "Target 'date::date-tz' not found after building ${DEP_NAME}.")
        endif()

        # Make sure the install step is run. One way is to add it as a dependency to a common target,
        # or ensure that targets that depend on wazuh::date also indirectly depend on this install.
        # For now, the install command is there. We might need to make `wazuh::date` depend on it.


    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            SHA256 ${DEP_SHA256} # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(DATE_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(DATE_LIBRARY "${DEP_INSTALL_DIR}/lib/libdate.a") # Adjust as per package
        set(DATETZ_LIBRARY "${DEP_INSTALL_DIR}/lib/libdate-tz.a") # Adjust as per package

        if(EXISTS "${DATE_LIBRARY}")
            add_library(wazuh::date STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::date PROPERTIES
                IMPORTED_LOCATION "${DATE_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::date library not found at ${DATE_LIBRARY}.")
        endif()

        if(EXISTS "${DATETZ_LIBRARY}")
            add_library(wazuh::date-tz STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::date-tz PROPERTIES
                IMPORTED_LOCATION "${DATETZ_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${DATE_INCLUDE_DIR}"
                # INTERFACE_LINK_LIBRARIES wazuh::date # If separate linking needed
            )
            # The original vcpkg port for date links against Threads::Threads and on UWP against an additional lib.
            # find_package(Threads REQUIRED)
            # target_link_libraries(wazuh::date-tz INTERFACE Threads::Threads)

        else()
            message(WARNING "wazuh::date-tz library not found at ${DATETZ_LIBRARY}.")
        endif()

    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process date
wazuh_add_dependency_date()

# --- flatbuffers ---
function(wazuh_add_dependency_flatbuffers)
    set(DEP_NAME "flatbuffers")
    set(DEP_VERSION "24.3.25")
    set(DEP_SOURCE_URL "https://github.com/google/flatbuffers/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for flatbuffers v24.3.25.tar.gz
    set(DEP_SHA256 "539e33f8f84518785a84866808a8613408f9ab70f5ee1a01814f291b59300774")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    # Path for flatc executable, relative to install dir for binaries, or build dir for source
    set(FLATC_EXECUTABLE_NAME "flatc")
    if(WIN32)
        set(FLATC_EXECUTABLE_NAME "flatc.exe")
    endif()

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "Disable flatbuffers tests" FORCE)
        set(FLATBUFFERS_BUILD_SHAREDLIB OFF CACHE BOOL "Build flatbuffers static library" FORCE)
        set(FLATBUFFERS_BUILD_FLATC ON CACHE BOOL "Enable flatc build" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for flatbuffers" FORCE)
        set(FLATBUFFERS_INSTALL ON CACHE BOOL "Enable flatbuffers install targets" FORCE)

        message(STATUS "Configuring flatbuffers source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        # Install step to populate DEP_INSTALL_DIR with flatc, lib, headers
        # Ensure this target is unique or make it so.
        set(FLATBUFFERS_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${FLATBUFFERS_INSTALL_TARGET_NAME})
            add_custom_target(${FLATBUFFERS_INSTALL_TARGET_NAME} ALL
                COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                DEPENDS flatbuffers # Depends on the library target from flatbuffers' CMake
                COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
            )
        endif()

        if(TARGET flatbuffers)
            add_library(wazuh::flatbuffers ALIAS flatbuffers)
            add_dependencies(wazuh::flatbuffers ${FLATBUFFERS_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'flatbuffers' (library) not found after building ${DEP_NAME}.")
        endif()

        set(FLATC_EXE_PATH "${DEP_INSTALL_DIR}/bin/${FLATC_EXECUTABLE_NAME}")
        if(NOT TARGET wazuh::flatc)
            # Check if flatc target from add_subdirectory exists, it's often just 'flatc'
            if(TARGET flatc)
                 # If flatc executable target from the subdir build exists, make wazuh::flatc an ALIAS.
                 # This assumes 'flatc' target from flatbuffers' CMake is the executable.
                 # We still need it to be *installed* to the location we expect.
                 add_executable(wazuh::flatc ALIAS flatc)
                 add_dependencies(wazuh::flatc ${FLATBUFFERS_INSTALL_TARGET_NAME})
            else()
                # If no direct 'flatc' target, create an IMPORTED one pointing to installed location.
                add_executable(wazuh::flatc IMPORTED)
                set_target_properties(wazuh::flatc PROPERTIES IMPORTED_LOCATION "${FLATC_EXE_PATH}")
                add_dependencies(wazuh::flatc ${FLATBUFFERS_INSTALL_TARGET_NAME})
            endif()
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(FLATBUFFERS_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(FLATBUFFERS_LIBRARY "${DEP_INSTALL_DIR}/lib/libflatbuffers.a") # Adjust as per package
        set(FLATC_EXE_PATH "${DEP_INSTALL_DIR}/bin/${FLATC_EXECUTABLE_NAME}")

        if(EXISTS "${FLATBUFFERS_LIBRARY}")
            add_library(wazuh::flatbuffers STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::flatbuffers PROPERTIES
                IMPORTED_LOCATION "${FLATBUFFERS_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FLATBUFFERS_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::flatbuffers library not found at ${FLATBUFFERS_LIBRARY}.")
        endif()

        if(EXISTS "${FLATC_EXE_PATH}")
            if(NOT TARGET wazuh::flatc)
                add_executable(wazuh::flatc IMPORTED GLOBAL) # GLOBAL for wazuh::flatc
                set_target_properties(wazuh::flatc PROPERTIES IMPORTED_LOCATION "${FLATC_EXE_PATH}")
            endif()
        else()
            message(WARNING "wazuh::flatc executable not found at ${FLATC_EXE_PATH}. S3 package might be missing it.")
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process flatbuffers
wazuh_add_dependency_flatbuffers()

# --- fast-float ---
function(wazuh_add_dependency_fast_float)
    set(DEP_NAME "fast-float")
    set(DEP_VERSION "6.1.4")
    set(DEP_SOURCE_URL "https://github.com/fastfloat/fast_float/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for fast_float v6.1.4.tar.gz
    set(DEP_SHA256 "f9978a7e6c22401f72a7aa537c3c77ef990e143e80309085e290b66318a4e9ac")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(FASTFLOAT_TEST OFF CACHE BOOL "Disable fast-float tests" FORCE)
        set(FASTFLOAT_BENCHMARK OFF CACHE BOOL "Disable fast-float benchmarks" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build fast-float static library" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for fast-float" FORCE)

        message(STATUS "Configuring fast-float source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(FASTFLOAT_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${FASTFLOAT_INSTALL_TARGET_NAME})
            # The target from fast_float's CMake is typically 'fast_float' for the library.
            if(TARGET fast_float)
                add_custom_target(${FASTFLOAT_INSTALL_TARGET_NAME} ALL
                   COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                   DEPENDS fast_float
                   COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                )
            else()
                 message(FATAL_ERROR "Target 'fast_float' (library) not found for ${DEP_NAME} before creating install step.")
            endif()
        endif()

        if(TARGET fast_float)
            add_library(wazuh::fast-float ALIAS fast_float)
            add_dependencies(wazuh::fast-float ${FASTFLOAT_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'fast_float' (library) not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(FASTFLOAT_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(FASTFLOAT_LIBRARY "${DEP_INSTALL_DIR}/lib/libfast_float.a") # Adjust as per package

        if(EXISTS "${FASTFLOAT_LIBRARY}")
            add_library(wazuh::fast-float STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::fast-float PROPERTIES
                IMPORTED_LOCATION "${FASTFLOAT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FASTFLOAT_INCLUDE_DIR}")
        else()
            # Fallback to header-only interface library if lib is not found but headers are
            if(IS_DIRECTORY "${FASTFLOAT_INCLUDE_DIR}/fast_float")
                message(WARNING "fast-float library not found at ${FASTFLOAT_LIBRARY}. Using as header-only.")
                add_library(wazuh::fast-float INTERFACE IMPORTED GLOBAL) # GLOBAL for interface too
                set_target_properties(wazuh::fast-float PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${FASTFLOAT_INCLUDE_DIR}")
            else()
                message(WARNING "wazuh::fast-float library and headers not found at ${DEP_INSTALL_DIR}. S3 package might be missing or structured differently.")
            endif()
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process fast-float
wazuh_add_dependency_fast_float()

# --- fmt ---
function(wazuh_add_dependency_fmt)
    set(DEP_NAME "fmt")
    set(DEP_VERSION "8.1.1")
    set(DEP_SOURCE_URL "https://github.com/fmtlib/fmt/archive/refs/tags/${DEP_VERSION}.tar.gz") # fmt does not use 'v' prefix for this version tag
    # SHA256 for fmt 8.1.1.tar.gz
    set(DEP_SHA256 "c03d242eab712136800c8a395a334ce09ce9f871e3d0f5ed9aa91d5cb667588a")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(FMT_TEST OFF CACHE BOOL "Disable fmt tests" FORCE)
        set(FMT_DOC OFF CACHE BOOL "Disable fmt documentation" FORCE)
        set(FMT_INSTALL ON CACHE BOOL "Enable fmt install target" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build fmt static library" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for fmt" FORCE)

        message(STATUS "Configuring fmt source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(FMT_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${FMT_INSTALL_TARGET_NAME})
            # The target from fmt's CMake for the static library is 'fmt'.
            if(TARGET fmt)
                add_custom_target(${FMT_INSTALL_TARGET_NAME} ALL
                   COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                   DEPENDS fmt
                   COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                )
            else()
                message(FATAL_ERROR "Target 'fmt' (library) not found for ${DEP_NAME} before creating install step.")
            endif()
        endif()

        if(TARGET fmt)
            add_library(wazuh::fmt ALIAS fmt)
            add_dependencies(wazuh::fmt ${FMT_INSTALL_TARGET_NAME})
            # fmtlib also provides fmt_header_only as an INTERFACE target. (fmt::fmt-header-only in vcpkg)
            # It seems the actual target name is 'fmt_header_only' from their CMake.
            if(TARGET fmt_header_only AND NOT TARGET wazuh::fmt-header-only)
                add_library(wazuh::fmt-header-only ALIAS fmt_header_only)
                # This is an interface target, so no build dependency on install step needed beyond what fmt itself does.
            endif()
        else()
            message(FATAL_ERROR "Target 'fmt' (library) not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(FMT_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(FMT_LIBRARY "${DEP_INSTALL_DIR}/lib/libfmt.a") # Adjust as per package

        if(EXISTS "${FMT_LIBRARY}")
            add_library(wazuh::fmt STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::fmt PROPERTIES
                IMPORTED_LOCATION "${FMT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
        else()
            # Fallback for header-only fmt if lib is not found but headers exist
            if(IS_DIRECTORY "${FMT_INCLUDE_DIR}/fmt")
                message(WARNING "fmt library ${FMT_LIBRARY} not found. Using fmt as header-only.")
                add_library(wazuh::fmt INTERFACE IMPORTED GLOBAL)
                set_target_properties(wazuh::fmt PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
            else()
                message(WARNING "wazuh::fmt library and headers not found at ${DEP_INSTALL_DIR}. S3 package might be missing or structured differently.")
            endif()
        endif()

        # Provide wazuh::fmt-header-only for binary mode as well, pointing to same include dir
        # Ensure it's only created if not already by some other means (e.g. if wazuh::fmt was created as INTERFACE)
        if(NOT TARGET wazuh::fmt-header-only)
            if(IS_DIRECTORY "${FMT_INCLUDE_DIR}/fmt")
                add_library(wazuh::fmt-header-only INTERFACE IMPORTED GLOBAL)
                set_target_properties(wazuh::fmt-header-only PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${FMT_INCLUDE_DIR}")
            endif()
        # If wazuh::fmt itself became an INTERFACE library, then wazuh::fmt-header-only is redundant unless named differently.
        # For clarity, if wazuh::fmt is a true compiled lib, wazuh::fmt-header-only offers the interface separately.
        # If wazuh::fmt is already an INTERFACE lib (fallback), then wazuh::fmt-header-only points to the same thing.
        # This logic seems fine.
        endif()

    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process fmt
wazuh_add_dependency_fmt()

# --- googletest (gtest & gmock) ---
function(wazuh_add_dependency_googletest)
    if(NOT ENGINE_BUILD_TEST)
        message(STATUS "Skipping googletest dependency: ENGINE_BUILD_TEST is OFF.")
        return()
    endif()

    set(DEP_NAME "googletest") # googletest is the repository name
    set(DEP_VERSION "1.15.0")
    set(DEP_SOURCE_URL "https://github.com/google/googletest/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for googletest v1.15.0.tar.gz
    set(DEP_SHA256 "2b9f5009c27c27614360a1776e1dbbd10680eca960466c77c30c40878d001598")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(BUILD_GMOCK ON CACHE BOOL "Build gmock along with gtest" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "Do not install gtest/gmock, use from build tree" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build gtest/gmock static libraries" FORCE)
        # CMAKE_INSTALL_PREFIX is less critical here since INSTALL_GTEST=OFF, but set for consistency
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for googletest (not used if INSTALL_GTEST=OFF)" FORCE)

        message(STATUS "Configuring googletest source build.")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(GTEST_TARGETS gtest gtest_main gmock gmock_main)
        foreach(GTEST_TARGET ${GTEST_TARGETS})
            if(TARGET ${GTEST_TARGET})
                add_library(wazuh::${GTEST_TARGET} ALIAS ${GTEST_TARGET})
            else()
                message(FATAL_ERROR "Target '${GTEST_TARGET}' not found after building ${DEP_NAME}.")
            endif()
        endforeach()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(GTEST_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(GTEST_LIB_DIR "${DEP_INSTALL_DIR}/lib")

        set(GTEST_TARGET_INFO
            gtest;libgtest.a
            gtest_main;libgtest_main.a
            gmock;libgmock.a
            gmock_main;libgmock_main.a
        )

        foreach(INFO ${GTEST_TARGET_INFO})
            string(REPLACE ";" ";" TARGET_NAME LIB_FILENAME ${INFO})
            set(FULL_LIB_PATH "${GTEST_LIB_DIR}/${LIB_FILENAME}")

            if(EXISTS "${FULL_LIB_PATH}")
                add_library(wazuh::${TARGET_NAME} STATIC IMPORTED GLOBAL)
                set_target_properties(wazuh::${TARGET_NAME} PROPERTIES
                    IMPORTED_LOCATION "${FULL_LIB_PATH}"
                    INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}")
                # find_package(Threads) # Should be done once globally if needed
                # if(TARGET Threads::Threads)
                #    target_link_libraries(wazuh::${TARGET_NAME} INTERFACE Threads::Threads)
                # endif()
            else()
                message(WARNING "wazuh::${TARGET_NAME} library not found at ${FULL_LIB_PATH}. S3 package might be missing or structured differently.")
            endif()
        endforeach()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process googletest
wazuh_add_dependency_googletest()

# --- libmaxminddb ---
function(wazuh_add_dependency_libmaxminddb)
    set(DEP_NAME "libmaxminddb")
    set(DEP_VERSION "1.9.1")
    set(DEP_SOURCE_URL "https://github.com/maxmind/libmaxminddb/archive/refs/tags/${DEP_VERSION}.tar.gz") # libmaxminddb does not use 'v' prefix for this version tag
    # SHA256 for libmaxminddb 1.9.1.tar.gz
    set(DEP_SHA256 "1ca00db38a900790d8d999e67902787f1aa9fed8d11c3935468f9b98f890eb77")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(BUILD_TESTING OFF CACHE BOOL "Disable libmaxminddb tests" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libmaxminddb static library" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for libmaxminddb" FORCE)

        message(STATUS "Configuring libmaxminddb source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(MAXMINDDB_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${MAXMINDDB_INSTALL_TARGET_NAME})
            # The target from libmaxminddb's CMake for the static library is 'maxminddb'.
            if(TARGET maxminddb)
                add_custom_target(${MAXMINDDB_INSTALL_TARGET_NAME} ALL
                   COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                   DEPENDS maxminddb
                   COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                )
            else()
                message(FATAL_ERROR "Target 'maxminddb' (library) not found for ${DEP_NAME} before creating install step.")
            endif()
        endif()

        if(TARGET maxminddb)
            add_library(wazuh::maxminddb ALIAS maxminddb)
            add_dependencies(wazuh::maxminddb ${MAXMINDDB_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'maxminddb' (library) not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(MAXMINDDB_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(MAXMINDDB_LIBRARY "${DEP_INSTALL_DIR}/lib/libmaxminddb.a") # Adjust as per package

        if(EXISTS "${MAXMINDDB_LIBRARY}")
            add_library(wazuh::maxminddb STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::maxminddb PROPERTIES
                IMPORTED_LOCATION "${MAXMINDDB_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${MAXMINDDB_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::maxminddb library not found at ${MAXMINDDB_LIBRARY}. S3 package might be missing or structured differently.")
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process libmaxminddb
wazuh_add_dependency_libmaxminddb()

# --- opentelemetry-cpp ---
function(wazuh_add_dependency_opentelemetry_cpp)
    set(DEP_NAME "opentelemetry-cpp")
    set(DEP_VERSION "1.8.3")
    set(DEP_SOURCE_URL "https://github.com/open-telemetry/opentelemetry-cpp/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for opentelemetry-cpp v1.8.3.tar.gz
    set(DEP_SHA256 "384e2b07f8cf9dda4575a599a4e79704f00e6a8de533a19a9008945258742341")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    # Define the components we expect and the actual library names produced by opentelemetry-cpp's build
    set(OTEL_COMPONENTS
        api;opentelemetry_api
        sdk;opentelemetry_sdk # This is a conceptual group; actual targets are more specific
        # Specific SDK components often include:
        # opentelemetry_trace_sdk (covered by opentelemetry_sdk convention in some builds)
        # opentelemetry_common
        # For v1.8.3, common targets are:
        # opentelemetry_api
        # opentelemetry_common
        # opentelemetry_resources
        # opentelemetry_trace         (for core SDK trace components)
        # opentelemetry_logs          (for core SDK log components)
        # opentelemetry_metrics       (for core SDK metric components)
        # Let's adjust based on typical target names rather than conceptual "sdk"
        common;opentelemetry_common
        resources;opentelemetry_resources
        trace;opentelemetry_trace # Corresponds to the main SDK part for tracing
        logs;opentelemetry_logs # For logs SDK
        metrics;opentelemetry_metrics # For metrics SDK
    )
    # The ALIAS names will be wazuh::opentelemetry-cpp::api, wazuh::opentelemetry-cpp::common etc.

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(WITH_TESTS OFF CACHE BOOL "Disable opentelemetry-cpp tests" FORCE)
        set(WITH_EXAMPLES OFF CACHE BOOL "Disable opentelemetry-cpp examples" FORCE)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build opentelemetry-cpp static libraries" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for opentelemetry-cpp" FORCE)
        # Ensure specific components are built if necessary (OTEL's CMake has options like WITH_LOGS_PREVIEW, etc.)
        # For v1.8.3, most core components (api, sdk parts) are built by default unless disabled.
        # We want API, SDK (trace, logs, metrics, resources, common).
        # Check OTEL CMake options: WITH_API_ONLY=OFF (default), WITH_SDK=ON (default),
        # WITH_LOGS_PREVIEW=ON (might be needed for logs), WITH_METRICS_PREVIEW=ON (for metrics)
        set(WITH_LOGS_PREVIEW ON CACHE BOOL "Enable OpenTelemetry Logs SDK (preview)" FORCE)
        set(WITH_METRICS_PREVIEW ON CACHE BOOL "Enable OpenTelemetry Metrics SDK (preview)" FORCE)

        message(STATUS "Configuring opentelemetry-cpp source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(OTEL_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${OTEL_INSTALL_TARGET_NAME})
            # The install target should build and install all necessary components.
            # Check if there's a single target that builds all required libs, e.g. 'all' or specific ones.
            # For OTEL, 'install' itself should depend on the built libraries.
            add_custom_target(${OTEL_INSTALL_TARGET_NAME} ALL
                COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
            )
        endif()

        foreach(COMPONENT_INFO ${OTEL_COMPONENTS})
            string(REPLACE ";" ";" ALIAS_SUFFIX ACTUAL_TARGET_NAME ${COMPONENT_INFO})
            set(ALIAS_TARGET_NAME "wazuh::opentelemetry-cpp::${ALIAS_SUFFIX}")

            if(TARGET ${ACTUAL_TARGET_NAME})
                add_library(${ALIAS_TARGET_NAME} ALIAS ${ACTUAL_TARGET_NAME})
                add_dependencies(${ALIAS_TARGET_NAME} ${OTEL_INSTALL_TARGET_NAME})
            else()
                message(WARNING "Target '${ACTUAL_TARGET_NAME}' for OTEL component '${ALIAS_SUFFIX}' not found. Alias ${ALIAS_TARGET_NAME} not created.")
            endif()
        endforeach()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(OTEL_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(OTEL_LIB_DIR "${DEP_INSTALL_DIR}/lib")

        foreach(COMPONENT_INFO ${OTEL_COMPONENTS})
            string(REPLACE ";" ";" ALIAS_SUFFIX ACTUAL_TARGET_NAME ${COMPONENT_INFO})
            set(LIB_FILENAME "lib${ACTUAL_TARGET_NAME}.a")
            set(FULL_LIB_PATH "${OTEL_LIB_DIR}/${LIB_FILENAME}")
            set(ALIAS_TARGET_NAME "wazuh::opentelemetry-cpp::${ALIAS_SUFFIX}")

            if(EXISTS "${FULL_LIB_PATH}")
                add_library(${ALIAS_TARGET_NAME} STATIC IMPORTED GLOBAL)
                set_target_properties(${ALIAS_TARGET_NAME} PROPERTIES
                    IMPORTED_LOCATION "${FULL_LIB_PATH}"
                    INTERFACE_INCLUDE_DIRECTORIES "${OTEL_INCLUDE_DIR}")
                # Note: INTERFACE_LINK_LIBRARIES for inter-dependencies (e.g., sdk parts on api, common)
                # needs to be handled. For example, trace, logs, metrics typically depend on api and common.
                # This is complex and depends on the actual library structure.
                # Example: if 'trace' depends on 'api' and 'common':
                # if(${ALIAS_SUFFIX} STREQUAL "trace")
                #   target_link_libraries(${ALIAS_TARGET_NAME} INTERFACE wazuh::opentelemetry-cpp::api wazuh::opentelemetry-cpp::common)
                # endif()
            else()
                message(WARNING "${ALIAS_TARGET_NAME} library not found at ${FULL_LIB_PATH}. S3 package might be missing component '${ACTUAL_TARGET_NAME}'.")
            endif()
        endforeach()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process opentelemetry-cpp
wazuh_add_dependency_opentelemetry_cpp()

# --- protobuf ---
function(wazuh_add_dependency_protobuf)
    set(DEP_NAME "protobuf")
    set(DEP_VERSION_TAG "v21.12") # Corresponds to 3.21.12
    set(DEP_VERSION "3.21.12")    # Actual version number
    set(DEP_SOURCE_URL "https://github.com/protocolbuffers/protobuf/archive/refs/tags/${DEP_VERSION_TAG}.tar.gz")
    # SHA256 for protobuf v21.12 (3.21.12).tar.gz
    set(DEP_SHA256 "d90cbb50d0c98f407f0d99f6f2b1e9e91c9770cb22dd2d5b6816aff4383eac27")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION_TAG}-src") # Use tag for source dir consistency
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION_TAG}-build")

    set(PROTOC_EXECUTABLE_NAME "protoc")
    if(WIN32)
        set(PROTOC_EXECUTABLE_NAME "protoc.exe")
    endif()

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION_TAG} # Use tag for download
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR} # Extracts to root of protobuf source
        )

        # Set CACHE variables before add_subdirectory
        set(protobuf_BUILD_TESTS OFF CACHE BOOL "Disable protobuf tests" FORCE)
        set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "Build protobuf static libraries" FORCE)
        set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "Build protoc compiler" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for protobuf" FORCE)

        message(STATUS "Configuring protobuf source build. Install prefix: ${DEP_INSTALL_DIR}")
        # The main CMakeLists.txt for protobuf is in a 'cmake' subdirectory of the extracted sources.
        add_subdirectory(${DEP_SOURCE_DIR}/cmake ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(PROTOBUF_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${PROTOBUF_INSTALL_TARGET_NAME})
            if(TARGET libprotobuf AND TARGET protoc)
                add_custom_target(${PROTOBUF_INSTALL_TARGET_NAME} ALL
                   COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                   DEPENDS libprotobuf protoc # Depends on the library and compiler targets
                   COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                )
            else()
                message(FATAL_ERROR "Targets 'libprotobuf' and/or 'protoc' not found for ${DEP_NAME} before creating install step.")
            endif()
        endif()

        if(TARGET libprotobuf)
            add_library(wazuh::libprotobuf ALIAS libprotobuf)
            add_dependencies(wazuh::libprotobuf ${PROTOBUF_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'libprotobuf' not found after building ${DEP_NAME}.")
        endif()

        if(TARGET protoc)
            # 'protoc' is an executable target. We create an alias to it.
            # The actual executable will be available after the install step at DEP_INSTALL_DIR/bin.
            add_executable(wazuh::protoc ALIAS protoc)
            add_dependencies(wazuh::protoc ${PROTOBUF_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'protoc' (executable) not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION} # Use numerical version for S3 package
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(PROTOBUF_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(PROTOBUF_LIBRARY "${DEP_INSTALL_DIR}/lib/libprotobuf.a") # Adjust as per package
        set(PROTOC_EXE_PATH "${DEP_INSTALL_DIR}/bin/${PROTOC_EXECUTABLE_NAME}")

        if(EXISTS "${PROTOBUF_LIBRARY}")
            add_library(wazuh::libprotobuf STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::libprotobuf PROPERTIES
                IMPORTED_LOCATION "${PROTOBUF_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_INCLUDE_DIR}")
            # find_package(Threads)
            # if(TARGET Threads::Threads)
            #    target_link_libraries(wazuh::libprotobuf INTERFACE Threads::Threads)
            # endif()
        else()
            message(WARNING "wazuh::libprotobuf library not found at ${PROTOBUF_LIBRARY}.")
        endif()

        if(EXISTS "${PROTOC_EXE_PATH}")
            if(NOT TARGET wazuh::protoc)
                add_executable(wazuh::protoc IMPORTED)
                set_target_properties(wazuh::protoc PROPERTIES IMPORTED_LOCATION "${PROTOC_EXE_PATH}")
            endif()
        else()
            message(WARNING "wazuh::protoc executable not found at ${PROTOC_EXE_PATH}. S3 package might be missing it.")
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process protobuf
wazuh_add_dependency_protobuf()

# --- pugixml ---
function(wazuh_add_dependency_pugixml)
    set(DEP_NAME "pugixml")
    set(DEP_VERSION "1.14")
    set(DEP_SOURCE_URL "https://github.com/zeux/pugixml/archive/refs/tags/v${DEP_VERSION}.tar.gz")
    # SHA256 for pugixml v1.14.tar.gz
    set(DEP_SHA256 "531d89aac9a912de704de690426310253337310f39773810f09a0f5262d6606c")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build pugixml static library" FORCE)
        set(PUGIXML_BUILD_TESTS OFF CACHE BOOL "Disable pugixml tests" FORCE)
        set(PUGIXML_BUILD_SAMPLES OFF CACHE BOOL "Disable pugixml samples" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for pugixml" FORCE)

        message(STATUS "Configuring pugixml source build. Install prefix: ${DEP_INSTALL_DIR}")
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(PUGIXML_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${PUGIXML_INSTALL_TARGET_NAME})
            # Target from pugixml's CMake is 'pugixml'.
            if(TARGET pugixml)
                add_custom_target(${PUGIXML_INSTALL_TARGET_NAME} ALL
                   COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
                   DEPENDS pugixml
                   COMMENT "Installing ${DEP_NAME} to ${DEP_INSTALL_DIR}"
                )
            else()
                message(FATAL_ERROR "Target 'pugixml' (library) not found for ${DEP_NAME} before creating install step.")
            endif()
        endif()

        if(TARGET pugixml)
            add_library(wazuh::pugixml ALIAS pugixml)
            add_dependencies(wazuh::pugixml ${PUGIXML_INSTALL_TARGET_NAME})
        else()
            message(FATAL_ERROR "Target 'pugixml' (library) not found after building ${DEP_NAME}.")
        endif()

    else() # Binary mode
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary package
            EXTRACT_DIR ${DEP_INSTALL_DIR}
        )

        set(PUGIXML_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        set(PUGIXML_LIBRARY "${DEP_INSTALL_DIR}/lib/libpugixml.a") # Adjust as per package

        if(EXISTS "${PUGIXML_LIBRARY}")
            add_library(wazuh::pugixml STATIC IMPORTED GLOBAL)
            set_target_properties(wazuh::pugixml PROPERTIES
                IMPORTED_LOCATION "${PUGIXML_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${PUGIXML_INCLUDE_DIR}")
        else()
            message(WARNING "wazuh::pugixml library not found at ${PUGIXML_LIBRARY}. S3 package might be missing or structured differently.")
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME}")
endfunction()

# Call the function to process pugixml
wazuh_add_dependency_pugixml()

# --- RapidJSON ---
function(wazuh_add_dependency_rapidjson)
    set(DEP_NAME "RapidJSON") # CMake package name is RapidJSON
    set(DEP_COMMIT "a95e013b97ca6523f32da23f5095fcc9dd6067e5")
    set(DEP_VERSION "a95e013") # Short version from commit
    set(DEP_SOURCE_URL "https://github.com/Tencent/rapidjson/archive/${DEP_COMMIT}.tar.gz")
    # SHA256 for Tencent/rapidjson commit a95e013b97ca6523f32da23f5095fcc9dd6067e5 tarball
    set(DEP_SHA256 "6f9968e41c61065031e00dca13330f32870f58f09cc0a303fd988a5f700e4179")
    set(DEP_S3_URL "${WAZUH_S3_BASE_URL}/${DEP_NAME}-${DEP_VERSION}-headers.tar.gz")

    set(DEP_SOURCE_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-src")
    set(DEP_INSTALL_DIR "${WAZUH_EXTERN_INSTALL_DIR}/${DEP_NAME}")
    set(DEP_BUILD_DIR "${WAZUH_EXTERN_DIR}/${DEP_NAME}-${DEP_VERSION}-build")

    if(WAZUH_EXTERNAL_SRC_ONLY)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_SOURCE_URL}
            SHA256 ${DEP_SHA256}
            EXTRACT_DIR ${DEP_SOURCE_DIR}
        )

        # Set CACHE variables before add_subdirectory
        set(RAPIDJSON_BUILD_TESTS OFF CACHE BOOL "Disable RapidJSON tests" FORCE)
        set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE BOOL "Disable RapidJSON examples" FORCE)
        set(RAPIDJSON_BUILD_DOC OFF CACHE BOOL "Disable RapidJSON documentation" FORCE)
        set(RAPIDJSON_INSTALL ON CACHE BOOL "Enable RapidJSON install target" FORCE)
        set(CMAKE_INSTALL_PREFIX ${DEP_INSTALL_DIR} CACHE PATH "Install prefix for RapidJSON" FORCE)

        message(STATUS "Configuring RapidJSON source. Install prefix: ${DEP_INSTALL_DIR}")
        # RapidJSON's CMakeLists.txt is at the root of its source.
        add_subdirectory(${DEP_SOURCE_DIR} ${DEP_BUILD_DIR} EXCLUDE_FROM_ALL)

        set(RAPIDJSON_INSTALL_TARGET_NAME "${DEP_NAME}_install_step")
        if(NOT TARGET ${RAPIDJSON_INSTALL_TARGET_NAME})
             # For header-only library with install rules, the install target itself is sufficient.
             # Check if add_subdirectory created an 'install' target or specific RapidJSON target.
             # RapidJSON's CMake might create an interface target like 'RapidJSON'.
             # If it does, we can depend on that for the install command.
             # For now, assume the install target in the build directory is enough.
            add_custom_target(${RAPIDJSON_INSTALL_TARGET_NAME} ALL
               COMMAND ${CMAKE_COMMAND} --build ${DEP_BUILD_DIR} --target install --config ${CMAKE_BUILD_TYPE}
               COMMENT "Installing ${DEP_NAME} headers to ${DEP_INSTALL_DIR}"
            )
        endif()

        # Define an interface library. Headers will be in ${DEP_INSTALL_DIR}/include/rapidjson
        add_library(wazuh::RapidJSON INTERFACE) # Match original find_package name
        target_include_directories(wazuh::RapidJSON INTERFACE "${DEP_INSTALL_DIR}/include")
        add_dependencies(wazuh::RapidJSON ${RAPIDJSON_INSTALL_TARGET_NAME}) # Ensure headers are "installed"

    else() # Binary mode (downloading headers from S3)
        wazuh_download_and_extract(
            NAME ${DEP_NAME}
            VERSION ${DEP_VERSION}
            URL ${DEP_S3_URL}
            # SHA256 for binary (header) package
            EXTRACT_DIR ${DEP_INSTALL_DIR} # Headers will be extracted to DEP_INSTALL_DIR/include/rapidjson
        )

        add_library(wazuh::RapidJSON INTERFACE IMPORTED GLOBAL)
        # Headers are expected to be in ${DEP_INSTALL_DIR}/include (rapidjson subdirectory created by its install)
        set(RAPIDJSON_INCLUDE_DIR "${DEP_INSTALL_DIR}/include")
        if(IS_DIRECTORY "${RAPIDJSON_INCLUDE_DIR}/rapidjson")
            set_target_properties(wazuh::RapidJSON PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_INCLUDE_DIR}")
        else()
            # If rapidjson headers are directly in include, not in a rapidjson subdir
            if(IS_DIRECTORY "${RAPIDJSON_INCLUDE_DIR}")
                 set_target_properties(wazuh::RapidJSON PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_INCLUDE_DIR}")
                 message(WARNING "RapidJSON headers found directly in ${RAPIDJSON_INCLUDE_DIR}, not in a 'rapidjson' subdirectory. This might be unexpected.")
            else()
                 message(WARNING "wazuh::RapidJSON headers not found at ${RAPIDJSON_INCLUDE_DIR}/rapidjson or ${RAPIDJSON_INCLUDE_DIR}. S3 package might be missing or structured differently.")
            endif()
        endif()
    endif()
    message(STATUS "Processed dependency: ${DEP_NAME} (header-only)")
endfunction()

# Call the function to process RapidJSON
wazuh_add_dependency_rapidjson()

message(STATUS "WazuhExternalDependencies.cmake loaded.")
