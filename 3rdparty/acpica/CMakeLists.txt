cmake_minimum_required(VERSION 3.20)
project(acpica)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/lib/$<CONFIG>)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS 
    "*.h" 
    "*.c")

add_library(acpica STATIC ${SRC})
target_include_directories(acpica PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_compile_options(acpica PRIVATE -mno-red-zone -mno-sse)
target_compile_definitions(acpica PRIVATE ACPI_USE_LOCAL_CACHE _CYGWIN)