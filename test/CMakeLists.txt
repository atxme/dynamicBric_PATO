cmake_minimum_required(VERSION 3.12)

#force ninja generator
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

# Force Release build for tests
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Configuration types" FORCE)

# Option to enable TLS support
option(USE_TLS "Enable TLS support (requires WolfSSL)" OFF)

# Find WolfSSL if TLS is enabled
if(USE_TLS)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WOLFSSL wolfssl)
    
    if(NOT WOLFSSL_FOUND)
        message(FATAL_ERROR "USE_TLS is enabled but WolfSSL was not found. Please install WolfSSL or disable USE_TLS.")
    endif()
    
    message(STATUS "TLS support enabled, using WolfSSL")
else()
    message(STATUS "TLS support disabled")
endif()

project(dynamicBric_PATO_test 
    VERSION 1.0
    LANGUAGES C CXX
    DESCRIPTION "Dynamic Bric PATO Tests"
)

# Find GTest
find_package(GTest REQUIRED)
enable_testing()

# Force absolute path
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Recursively find all source files
file(GLOB_RECURSE ALL_C_SOURCES "${PROJECT_ROOT}/*.c")
file(GLOB_RECURSE ALL_CPP_SOURCES "${PROJECT_ROOT}/*.cpp")
file(GLOB_RECURSE ALL_HEADERS "${PROJECT_ROOT}/*.h")

# Create lists for different types of source files
set(TLS_SOURCES "")
set(NON_TLS_SOURCES "")
set(TEST_CPP_SOURCES "")

# Filter source files based on their paths
foreach(SOURCE_FILE ${ALL_C_SOURCES} ${ALL_CPP_SOURCES})
    # Skip external/vendor directories if they exist
    if(SOURCE_FILE MATCHES ".*/vendor/.*|.*/external/.*")
        continue()
    endif()
    
    # Process test files
    if(SOURCE_FILE MATCHES ".*/test/.*")
        # Include only test files from current directory for compilation
        if(SOURCE_FILE MATCHES "${CMAKE_CURRENT_SOURCE_DIR}/.*.cpp$")
            list(APPEND TEST_CPP_SOURCES ${SOURCE_FILE})
        endif()
        # Skip other test directories
        continue()
    endif()
    
    # Skip Raspberry Pi specific code
    if(SOURCE_FILE MATCHES ".*/xProtocol/.*")
        continue()
    endif()
    
    # Skip CMake compiler identification files
    if(SOURCE_FILE MATCHES ".*CMakeCCompilerId\\.c$|.*CMakeCXXCompilerId\\.cpp$")
        continue()
    endif()
    
    # Categorize TLS and non-TLS files
    if(SOURCE_FILE MATCHES ".*/tls/.*|.*/crypto/TLS/.*|.*/crypto/cipher/.*")
        list(APPEND TLS_SOURCES ${SOURCE_FILE})
    elseif(SOURCE_FILE MATCHES ".*/network/xNetwork\\.c$")
        # Standard network implementation (only include when TLS is disabled)
        if(NOT USE_TLS)
            list(APPEND NON_TLS_SOURCES ${SOURCE_FILE})
        endif()
    else()
        list(APPEND NON_TLS_SOURCES ${SOURCE_FILE})
    endif()
endforeach()

# Set up final sources list based on USE_TLS
if(USE_TLS)
    set(SOURCES ${NON_TLS_SOURCES} ${TLS_SOURCES} ${TEST_CPP_SOURCES})
    add_compile_definitions(USE_TLS)
else()
    set(SOURCES ${NON_TLS_SOURCES} ${TEST_CPP_SOURCES})
endif()

# Print source files information for debugging
list(LENGTH SOURCES SOURCE_COUNT)
list(LENGTH TLS_SOURCES TLS_SOURCE_COUNT)
list(LENGTH TEST_CPP_SOURCES TEST_COUNT)
message(STATUS "Found ${SOURCE_COUNT} total source files")
message(STATUS "TLS source files: ${TLS_SOURCE_COUNT}")
message(STATUS "Test source files: ${TEST_COUNT}")

# Create the test executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Add include directories (recursively find all directories)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_ROOT})

# Find all directories in the project and add them as include directories
file(GLOB_RECURSE ALL_DIRS LIST_DIRECTORIES true "${PROJECT_ROOT}/*")
foreach(DIR ${ALL_DIRS})
    if(IS_DIRECTORY "${DIR}" AND 
       NOT DIR MATCHES ".*/test/.*" AND 
       NOT DIR MATCHES ".*/xProtocol/.*" AND
       NOT DIR MATCHES ".*/vendor/.*" AND
       NOT DIR MATCHES ".*/external/.*")
        target_include_directories(${PROJECT_NAME} PRIVATE "${DIR}")
    endif()
endforeach()

# Include GTest directories
target_include_directories(${PROJECT_NAME} PRIVATE ${GTEST_INCLUDE_DIRS})

# Include WolfSSL directories if TLS is enabled
if(USE_TLS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${WOLFSSL_INCLUDE_DIRS})
endif()

# Compiler options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /O2 /Oi /Gy /MD
        /wd4100
        /wd4189
    )
    target_link_options(${PROJECT_NAME} PRIVATE /LTCG)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -O3 
        -march=native 
        -flto
        -Wno-variadic-macros
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unused-local-typedefs
    )
    target_link_options(${PROJECT_NAME} PRIVATE -flto)
endif()

# C and C++ standards
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Compilation definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NDEBUG
    _POSIX_C_SOURCE=200809L
    _GNU_SOURCE
    _XOPEN_SOURCE=700
    UNIT_TESTING  # Add this to enable any test-specific code
)

# Libraries
if(UNIX)
    # Base libraries
    set(UNIX_LIBS 
        GTest::GTest
        GTest::Main
        pthread
        rt
    )
    
    # Add WolfSSL library if TLS is enabled
    if(USE_TLS)
        list(APPEND UNIX_LIBS ${WOLFSSL_LIBRARIES} ssl crypto)
    endif()
    
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UNIX_LIBS})
endif()

# Configure GTest
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

# Configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  TLS Support: ${USE_TLS}")
message(STATUS "  C++ Standard: 20")
message(STATUS "  C Standard: 17")
if(USE_TLS)
    message(STATUS "  WolfSSL Include Dirs: ${WOLFSSL_INCLUDE_DIRS}")
    message(STATUS "  WolfSSL Libraries: ${WOLFSSL_LIBRARIES}")
endif()
