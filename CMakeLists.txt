cmake_minimum_required(VERSION 3.12)

# Set available build types
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")

set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

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

if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(dynamicBric_PATO 
    VERSION 1.0
    LANGUAGES C CXX
    DESCRIPTION "Dynamic Bric PATO Library"
)

# Debug configuration
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -O0")

# Release configuration
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG")

# RelWithDebInfo configuration
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")

# MinSizeRel configuration
set(CMAKE_C_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL} -Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")

# Force absolute path
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Recursively find all source files, excluding test directories
file(GLOB_RECURSE ALL_C_SOURCES "${PROJECT_ROOT}/*.c")
file(GLOB_RECURSE ALL_CPP_SOURCES "${PROJECT_ROOT}/*.cpp")
file(GLOB_RECURSE ALL_HEADERS "${PROJECT_ROOT}/*.h")

# Create lists for TLS-specific files
set(TLS_SOURCES "")
set(NON_TLS_SOURCES "")

# Filter out test files and separate TLS files
foreach(SOURCE_FILE ${ALL_C_SOURCES} ${ALL_CPP_SOURCES})
    # Skip test files
    if(SOURCE_FILE MATCHES ".*test.*")
        continue()
    endif()
    
    # Check if this is a TLS-related file
    if(SOURCE_FILE MATCHES ".*/crypto/TLS/.*|.*/crypto/cipher/.*")
        list(APPEND TLS_SOURCES ${SOURCE_FILE})
    else()
        list(APPEND NON_TLS_SOURCES ${SOURCE_FILE})
    endif()
endforeach()

# Set up final sources list based on USE_TLS
if(USE_TLS)
    set(SOURCES ${NON_TLS_SOURCES} ${TLS_SOURCES})
    add_compile_definitions(USE_TLS)
else()
    set(SOURCES ${NON_TLS_SOURCES})
endif()

# Print source files for debugging
list(LENGTH SOURCES SOURCE_COUNT)
list(LENGTH TLS_SOURCES TLS_SOURCE_COUNT)
message(STATUS "Found ${SOURCE_COUNT} source files")
message(STATUS "TLS source files: ${TLS_SOURCE_COUNT}")

# Create library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${ALL_HEADERS})

# Add include directories (the root and all subdirectories except test directories)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Find all directories in the project
file(GLOB_RECURSE ALL_DIRS LIST_DIRECTORIES true "${PROJECT_ROOT}/*")
foreach(DIR ${ALL_DIRS})
    if(IS_DIRECTORY "${DIR}" AND NOT DIR MATCHES ".*test.*")
        target_include_directories(${PROJECT_NAME} PRIVATE "${DIR}")
    endif()
endforeach()

# Include WolfSSL directories if TLS is enabled
if(USE_TLS)
    target_include_directories(${PROJECT_NAME} PRIVATE ${WOLFSSL_INCLUDE_DIRS})
endif()

# Compiler options
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DYNAMICBRIC_PATO_EXPORTS)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        $<$<CONFIG:Debug>:/Od /RTC1 /Zi /MDd>
        $<$<CONFIG:Release>:/O2 /Oi /Gy /MD>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Zi /MD>
        $<$<CONFIG:MinSizeRel>:/O1 /MD>
        /wd4100
        /wd4189
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:MinSizeRel>:/LTCG>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Debug>:-g3 -O0 -fno-omit-frame-pointer>
        $<$<CONFIG:Release>:-O3 -march=native -flto>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g -march=native>
        $<$<CONFIG:MinSizeRel>:-Os -march=native>
        -fdiagnostics-color=always
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unused-local-typedefs
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-flto>
        $<$<CONFIG:MinSizeRel>:-flto>
    )
endif()

# Add conditional definitions based on build type
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Debug>:DEBUG _DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG>
    $<$<CONFIG:MinSizeRel>:NDEBUG>
)

# C and C++ standards
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Unix specific settings
if(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _POSIX_C_SOURCE=200809L
        _GNU_SOURCE
        _XOPEN_SOURCE=700
    )
    
    # Base libraries
    set(UNIX_LIBS pthread rt)
    
    # Add WolfSSL library if TLS is enabled
    if(USE_TLS)
        list(APPEND UNIX_LIBS ${WOLFSSL_LIBRARIES} ssl crypto)
    endif()
    
    target_link_libraries(${PROJECT_NAME} PRIVATE ${UNIX_LIBS})
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers
foreach(HEADER ${ALL_HEADERS})
    # Skip test headers
    if(NOT HEADER MATCHES ".*test.*")
        file(RELATIVE_PATH REL_PATH ${PROJECT_ROOT} ${HEADER})
        get_filename_component(INSTALL_DIR ${REL_PATH} DIRECTORY)
        install(FILES ${HEADER} 
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${INSTALL_DIR})
    endif()
endforeach()

# Output configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  TLS Support: ${USE_TLS}")
message(STATUS "  C++ Standard: 20")
message(STATUS "  C Standard: 17")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
if(USE_TLS)
    message(STATUS "  WolfSSL Include Dirs: ${WOLFSSL_INCLUDE_DIRS}")
    message(STATUS "  WolfSSL Libraries: ${WOLFSSL_LIBRARIES}")
endif()
