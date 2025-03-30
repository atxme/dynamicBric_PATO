cmake_minimum_required(VERSION 3.12)

# Set available build types
set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel")

set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

# Option to enable/disable Raspberry Pi mode and version
option(ENABLE_RASPBERRY "Enable Raspberry Pi specific code" ON)
option(RPI_VERSION "Raspberry Pi Version (4 or 5)" 4)

# Required for libgpiod
if (RPI_VERSION)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPIOD REQUIRED libgpiod)
endif()

# Find WolfSSL
find_package(PkgConfig REQUIRED)
pkg_check_modules(WOLFSSL wolfssl)

if(NOT WOLFSSL_FOUND)
    message(WARNING "WolfSSL not found. Will try to use it anyway, but compilation might fail.")
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

# Debug paths and configuration
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Raspberry Pi mode: ${ENABLE_RASPBERRY}")
message(STATUS "Raspberry Pi version: ${RPI_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "GPIOD Include Dirs: ${GPIOD_INCLUDE_DIRS}")
message(STATUS "GPIOD Libraries: ${GPIOD_LIBRARIES}")

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

# Define base sources (always included)
set(BASE_SOURCES
    ${PROJECT_ROOT}/dynamicBric_PATO.cpp
    ${PROJECT_ROOT}/network/xNetwork.c
    ${PROJECT_ROOT}/assert/xAssert.c
    ${PROJECT_ROOT}/xOs/xTask.c
    ${PROJECT_ROOT}/xOs/xOsCritical.c
    ${PROJECT_ROOT}/xOs/xOsMutex.c
    ${PROJECT_ROOT}/xOs/xOsHorodateur.c
    ${PROJECT_ROOT}/timer/xTimer.c
    ${PROJECT_ROOT}/crypto/hash/xHash.c
    # ${PROJECT_ROOT}/crypto/cipher/xCipherEngine.c
    # ${PROJECT_ROOT}/crypto/TLS/xTlsEngine.c
    ${PROJECT_ROOT}/memory/xMemory.c
    ${PROJECT_ROOT}/timer/xTimer.c
    ${PROJECT_ROOT}/xLog/xLog.c
)

# Base headers
set(BASE_HEADERS
    ${PROJECT_ROOT}/dynamicBric_PATO.h
    ${PROJECT_ROOT}/network/xNetwork.h
    ${PROJECT_ROOT}/assert/xAssert.h
    ${PROJECT_ROOT}/xOs/xTask.h
    ${PROJECT_ROOT}/xOs/xOsCritical.h
    ${PROJECT_ROOT}/xOs/xOsMutex.h
    ${PROJECT_ROOT}/xOs/xOsHorodateur.h
    ${PROJECT_ROOT}/crypto/hash/xHash.h
    # ${PROJECT_ROOT}/crypto/cipher/xCipherEngine.h
    # ${PROJECT_ROOT}/crypto/TLS/xTlsEngine.h
    ${PROJECT_ROOT}/memory/xMemory.h
    ${PROJECT_ROOT}/timer/xTimer.h
    ${PROJECT_ROOT}/xLog/xLog.h
)

# Raspberry Pi specific sources
set(RASPBERRY_SOURCES
    ${PROJECT_ROOT}/xProtocol/xUart.c
    ${PROJECT_ROOT}/xProtocol/xSpi.c
    ${PROJECT_ROOT}/xProtocol/xGpio.c
    ${PROJECT_ROOT}/xProtocol/xI2c.c
    ${PROJECT_ROOT}/xProtocol/xPwm.c
)

# Raspberry Pi specific headers
set(RASPBERRY_HEADERS
    ${PROJECT_ROOT}/xProtocol/xUart.h
    ${PROJECT_ROOT}/xProtocol/xSpi.h
    ${PROJECT_ROOT}/xProtocol/xGpio.h
    ${PROJECT_ROOT}/xProtocol/xI2c.h
    ${PROJECT_ROOT}/xProtocol/xPwm.h
)

# Configure sources based on Raspberry Pi mode
if(ENABLE_RASPBERRY)
    set(SOURCES ${BASE_SOURCES} ${RASPBERRY_SOURCES})
    set(HEADERS ${BASE_HEADERS} ${RASPBERRY_HEADERS})
else()
    set(SOURCES ${BASE_SOURCES})
    set(HEADERS ${BASE_HEADERS})
endif()

# Create library
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Configure definitions based on Raspberry Pi mode
if(ENABLE_RASPBERRY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _RASPBERRY
        RPI_${RPI_VERSION}
    )
endif()

# Adjust include paths
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/network
        ${CMAKE_CURRENT_SOURCE_DIR}/assert
        ${CMAKE_CURRENT_SOURCE_DIR}/xOs
        ${CMAKE_CURRENT_SOURCE_DIR}/xProtocol
        ${CMAKE_CURRENT_SOURCE_DIR}/crypto
        ${GPIOD_INCLUDE_DIRS}
        ${WOLFSSL_INCLUDE_DIRS}
)

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
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        pthread 
        rt
        ssl
        crypto
        ${WOLFSSL_LIBRARIES}
        ${GPIOD_LIBRARIES}
    )
endif()

# Installation
include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# Output configuration summary
message(STATUS "Configuration Summary:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Raspberry Pi Support: ${ENABLE_RASPBERRY}")
message(STATUS "  Raspberry Pi Version: ${RPI_VERSION}")
message(STATUS "  C++ Standard: 20")
message(STATUS "  C Standard: 17")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  GPIOD Support: ${GPIOD_FOUND}")
message(STATUS "  WolfSSL Support: ${WOLFSSL_FOUND}")

# Add TLS examples if the directory exists
# if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/tls/CMakeLists.txt")
#     add_subdirectory(examples/tls)
# endif()
