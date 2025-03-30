cmake_minimum_required(VERSION 3.12)

# Find WolfSSL
find_package(PkgConfig REQUIRED)
pkg_check_modules(WOLFSSL wolfssl)

if(NOT WOLFSSL_FOUND)
    message(WARNING "WolfSSL not found. Will try to use it anyway, but compilation might fail.")
endif()

# Set TLS examples project
project(tls_examples
    VERSION 1.0
    LANGUAGES C
    DESCRIPTION "TLS Examples using xTlsEngine"
)

# Get the parent directory path
get_filename_component(PARENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)

# Client example
add_executable(tls_client_example
    ${CMAKE_CURRENT_SOURCE_DIR}/tls_client_example.c
)

# Server example
add_executable(tls_server_example
    ${CMAKE_CURRENT_SOURCE_DIR}/tls_server_example.c
)

# Include directories for both examples
foreach(target tls_client_example tls_server_example)
    target_include_directories(${target} PRIVATE
        ${PARENT_DIR}
        ${WOLFSSL_INCLUDE_DIRS}
    )
    
    # Link with the main library and required dependencies
    target_link_libraries(${target} PRIVATE
        dynamicBric_PATO
        pthread
        ${WOLFSSL_LIBRARIES}
    )
    
    # Set C standard
    set_target_properties(${target} PROPERTIES
        C_STANDARD 17
        C_STANDARD_REQUIRED ON
        C_EXTENSIONS OFF
    )
    
    # Compiler options
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /wd4100
            /wd4189
        )
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wno-unused-parameter
            -Wno-unused-variable
        )
    endif()
endforeach()

# Install targets
install(TARGETS tls_client_example tls_server_example
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/examples
)

# Output configuration
message(STATUS "Configured TLS examples:")
message(STATUS "  - tls_client_example")
message(STATUS "  - tls_server_example") 