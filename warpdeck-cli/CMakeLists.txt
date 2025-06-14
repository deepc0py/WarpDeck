cmake_minimum_required(VERSION 3.15)
project(warpdeck-cli VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libwarpdeck
find_library(WARPDECK_LIBRARY 
    NAMES warpdeck libwarpdeck
    PATHS 
        ${CMAKE_SOURCE_DIR}/../libwarpdeck/build
        ${CMAKE_BINARY_DIR}/../libwarpdeck
        ${CMAKE_CURRENT_SOURCE_DIR}/../libwarpdeck/build
        /usr/local/lib
        /usr/lib
        /usr/lib/x86_64-linux-gnu
        /usr/lib64
    DOC "WarpDeck library path"
)

if(NOT WARPDECK_LIBRARY)
    message(FATAL_ERROR "libwarpdeck not found. Please build libwarpdeck first.")
endif()

# Find required packages (same as libwarpdeck)
find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Platform detection
if(APPLE)
    set(WARPDECK_PLATFORM_MACOS ON)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    set(PLATFORM_LIBRARIES ${CORE_FOUNDATION_FRAMEWORK} ${FOUNDATION_FRAMEWORK})
elseif(UNIX AND NOT APPLE)
    set(WARPDECK_PLATFORM_LINUX ON)
    pkg_check_modules(AVAHI REQUIRED avahi-client)
    set(PLATFORM_LIBRARIES ${AVAHI_LIBRARIES})
    include_directories(${AVAHI_INCLUDE_DIRS})
    link_directories(${AVAHI_LIBRARY_DIRS})
endif()

# Add third-party dependencies (same as libwarpdeck for linking)
include(FetchContent)

# Fetch cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

# Fetch nlohmann/json for JSON handling
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# CLI source files
set(CLI_SOURCES
    src/main.cpp
    src/cli_application.cpp
    src/command_parser.cpp
    src/interactive_ui.cpp
)

# Create the CLI executable
add_executable(warpdeck ${CLI_SOURCES})

# Include directories
target_include_directories(warpdeck PRIVATE
    ${CMAKE_SOURCE_DIR}/../libwarpdeck/include
    src
)

# Link libraries
target_link_libraries(warpdeck PRIVATE
    ${WARPDECK_LIBRARY}
    OpenSSL::SSL 
    OpenSSL::Crypto
    Threads::Threads
    httplib::httplib
    nlohmann_json::nlohmann_json
    ${PLATFORM_LIBRARIES}
)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(warpdeck PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target
install(TARGETS warpdeck
    RUNTIME DESTINATION bin
)