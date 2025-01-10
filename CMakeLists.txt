cmake_minimum_required(VERSION 3.12)

set(CMAKE_GENERATOR "Ninja" CACHE INTERNAL "" FORCE)

# Définir la politique pour le format des informations de débogage MSVC
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(dynamicBric_PATO 
    VERSION 1.0
    LANGUAGES C CXX
    DESCRIPTION "Dynamic Bric PATO Library"

)

# Debug des chemins
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
file(GLOB_RECURSE ALL_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(FILE ${ALL_FILES})
    message(STATUS "Found file: ${FILE}")
endforeach()

# Forcer le chemin absolu
get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)

# Redéfinir les sources avec le chemin absolu
set(SOURCES
    ${PROJECT_ROOT}/dynamicBric_PATO.cpp
    ${PROJECT_ROOT}/network/core.c
    ${PROJECT_ROOT}/assert/xAssert.c
    ${PROJECT_ROOT}/xOs/xTask.c
    ${PROJECT_ROOT}/xOs/xOsCritical.c
)

set(HEADERS
    ${PROJECT_ROOT}/dynamicBric_PATO.h
    ${PROJECT_ROOT}/network/core.h
    ${PROJECT_ROOT}/assert/xAssert.h
    ${PROJECT_ROOT}/xOs/xTask.h
    ${PROJECT_ROOT}/xOs/xOsCritical.h
)

# Créer la bibliothèque avec les sources
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Ajuster les chemins d'inclusion
target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/network
        ${CMAKE_CURRENT_SOURCE_DIR}/assert
        ${CMAKE_CURRENT_SOURCE_DIR}/xOs
)



# Définir les options du compilateur de manière moderne
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        DYNAMICBRIC_PATO_EXPORTS
    )
    
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        $<$<CONFIG:Release>:/O2>
        $<$<CONFIG:Debug>:/Od /RTC1>
        /wd4100
        /wd4189
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Werror
        $<$<CONFIG:Release>:-O2>
        -fdiagnostics-color=always
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unused-local-typedefs
    )
endif()

# Définir les standards C et C++
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 17
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _POSIX_C_SOURCE=200809L
        _GNU_SOURCE
        _XOPEN_SOURCE=700
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread rt)
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