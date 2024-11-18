# CMakeList.txt : projet CMake pour dynamicBric_PATO, incluez la source et définissez
# la logique spécifique au projet ici.
#
cmake_minimum_required (VERSION 3.8)

# Activez Rechargement à chaud pour les compilateurs MSVC si cela est pris en charge.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("dynamicBric_PATO")

#inclure un directory
include_directories("network")
include_directories("assert")
include_directories("xOs")

#creation d'une ddl si windows et so sous linux
if (WIN32)
  add_compile_definitions(DYNAMICBRIC_PATO_EXPORTS)
endif()

add_library(dynamicBric_PATO SHARED "dynamicBric_PATO.cpp" "dynamicBric_PATO.h" "network/core.h" "network/core.c" "assert/xAssert.h" "assert/xAssert.c" "xOs/xTask.h" "xOs/xTask.c")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET dynamicBric_PATO PROPERTY CXX_STANDARD 20 C_STANDARD 20)
endif()

# TODO: Ajoutez des tests et installez des cibles si nécessaire.
