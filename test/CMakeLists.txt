cmake_minimum_required(VERSION 3.12)

# Force Release build for tests
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Configuration types" FORCE)

#compilation avec ninja
set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

# Option to enable/disable Raspberry Pi mode and version
option(ENABLE_RASPBERRY "Enable Raspberry Pi specific code" ON)
option(RPI_VERSION "Raspberry Pi Version (4 or 5)" 4)

# Required for libgpiod
if (RPI_VERSION)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPIOD REQUIRED libgpiod)
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

# Base sources (always included)
set(BASE_SOURCES
    ${PROJECT_ROOT}/dynamicBric_PATO.cpp
    ${PROJECT_ROOT}/network/xNetwork.c
    ${PROJECT_ROOT}/assert/xAssert.c
    ${PROJECT_ROOT}/xOs/xTask.c
    ${PROJECT_ROOT}/xOs/xOsCritical.c
    ${PROJECT_ROOT}/xOs/xOsMutex.c
    ${PROJECT_ROOT}/xOs/xOsHorodateur.c
    ${PROJECT_ROOT}/timer/xTimer.c
    ${PROJECT_ROOT}/hash/xHash.c
    ${PROJECT_ROOT}/memory/xMemory.c
    ${PROJECT_ROOT}/timer/xTimer.c
    ${PROJECT_ROOT}/xLog/xLog.c
    ${PROJECT_ROOT}/events/xEvent.c
    ${PROJECT_ROOT}/events/xEventQueue.c
    ${PROJECT_ROOT}/events/xEventHandler.c
)

# Raspberry Pi specific sources
set(RASPBERRY_SOURCES
    ${PROJECT_ROOT}/xProtocol/xUart.c
    ${PROJECT_ROOT}/xProtocol/xSpi.c
    ${PROJECT_ROOT}/xProtocol/xGpio.c
    ${PROJECT_ROOT}/xProtocol/xI2c.c
    ${PROJECT_ROOT}/xProtocol/xPwm.c
)

# Test sources
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_assert.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_critical.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_event_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_event_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_events.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_horodateur.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_mutex.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_network.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_tasks.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp

    # Ajoutez ici vos autres fichiers de test
)

# Configure sources based on Raspberry Pi mode
if(ENABLE_RASPBERRY)
    set(SOURCES ${BASE_SOURCES} ${RASPBERRY_SOURCES} ${TEST_SOURCES})
else()
    set(SOURCES ${BASE_SOURCES} ${TEST_SOURCES})
endif()

# Création de l'exécutable de test
add_executable(${PROJECT_NAME} ${SOURCES})

# Configuration des includes
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/network
    ${PROJECT_ROOT}/assert
    ${PROJECT_ROOT}/xOs
    ${PROJECT_ROOT}/xProtocol
    ${PROJECT_ROOT}/timer
    ${PROJECT_ROOT}/hash
    ${PROJECT_ROOT}/memory
    ${PROJECT_ROOT}/xLog
    ${PROJECT_ROOT}/events
    ${GTEST_INCLUDE_DIRS}
    ${GPIOD_INCLUDE_DIRS}
)

# Configure definitions based on Raspberry Pi mode
if(ENABLE_RASPBERRY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE 
        _RASPBERRY
        RPI_${RPI_VERSION}
    )
endif()

# Options de compilation
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
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unused-local-typedefs
    )
    target_link_options(${PROJECT_NAME} PRIVATE -flto)
endif()

# Configuration C/C++ standards
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Définitions de compilation
target_compile_definitions(${PROJECT_NAME} PRIVATE
    NDEBUG
    _POSIX_C_SOURCE=200809L
    _GNU_SOURCE
    _XOPEN_SOURCE=700
)

# Liaison avec les bibliothèques
if(UNIX)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        GTest::GTest
        GTest::Main
        pthread
        rt
        ssl
        crypto
        ${GPIOD_LIBRARIES}
    )
endif()

# Configuration des tests
include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})

# Messages de debug
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Raspberry Pi mode: ${ENABLE_RASPBERRY}")
message(STATUS "Raspberry Pi version: ${RPI_VERSION}")
message(STATUS "Test Sources: ${TEST_SOURCES}")
message(STATUS "Base Sources: ${BASE_SOURCES}")
if(ENABLE_RASPBERRY)
    message(STATUS "Raspberry Sources: ${RASPBERRY_SOURCES}")
endif()
message(STATUS "GPIOD Include Dirs: ${GPIOD_INCLUDE_DIRS}")
message(STATUS "GPIOD Libraries: ${GPIOD_LIBRARIES}")
