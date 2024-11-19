cmake_minimum_required(VERSION 3.12)

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

# Définir les sources
set(SOURCES
    dynamicBric_PATO.cpp
    network/core.c
    assert/xAssert.c
    xOs/xTask.c
    xOs/xOsCritical.c
)

set(HEADERS
    dynamicBric_PATO.h
    network/core.h
    assert/xAssert.h
    xOs/xTask.h
    xOs/xOsCritical.h
)

# Créer la bibliothèque partagée
add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})

# Configurer les dossiers d'inclusion de manière moderne
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
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