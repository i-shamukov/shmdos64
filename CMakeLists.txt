cmake_minimum_required(VERSION 3.20)
project(shmdos64 VERSION 1.0.0)


set(SHM_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(KERNEL_MODULE_INCLUDE_ROOT ${SHM_ROOT}/include)
set(KERNEL_MODULE_LIB_ROOT ${SHM_ROOT}/lib/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${SHM_ROOT}/bin/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${KERNEL_MODULE_LIB_ROOT})
set(EXTLIBS ${SHM_ROOT}/3rdparty)

set(CMAKE_VERBOSE_MAKEFILE on)

add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-std=c++17> $<$<COMPILE_LANGUAGE:C>:-std=c99>)


function(kernel_target_init target_name)
    target_include_directories(${target_name} PRIVATE ${KERNEL_MODULE_INCLUDE_ROOT})
    target_compile_options(${target_name} PRIVATE -masm=intel -fno-exceptions -fno-rtti -fno-threadsafe-statics -mcx16 -mno-red-zone -mno-sse)
endfunction()

function(kernel_executable exe_name)
    kernel_target_init(${exe_name})
    target_link_options(${exe_name} PRIVATE -m64 -nostdlib)
endfunction()

function(kernel_module module_name)
    kernel_executable(${module_name})
    target_link_directories(${module_name} PRIVATE ${KERNEL_MODULE_LIB_ROOT})
    target_link_libraries(${module_name} PRIVATE moduleinit)
    set_target_properties(${module_name} PROPERTIES IMPORT_SUFFIX ".a")
    set_target_properties(${module_name} PROPERTIES PREFIX "")
endfunction()

function(kernel_static_lib lib_name)
    kernel_target_init(${lib_name})
endfunction()

function(kernel_efi_module efi_name)
    kernel_executable(${efi_name})
    target_link_options(${efi_name} PRIVATE -Wl,--pic-executable -Wl,--subsystem,10 -Wl,--dll -e _Z5entryPvP16EFI_SYSTEM_TABLE)
    set_target_properties(${efi_name} PROPERTIES ENABLE_EXPORTS 0)
    set_target_properties(${efi_name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${SHM_ROOT}/garbage)
endfunction()

function(kernel_main main_module_name)
    kernel_executable(${main_module_name})
    target_link_options(${main_module_name} PRIVATE -Wl,--pic-executable -e _Z5entryP12KernelParams)
    set_target_properties(${main_module_name} PROPERTIES ENABLE_EXPORTS 1 IMPORT_SUFFIX ".a")
endfunction()

add_subdirectory(uefi)
add_subdirectory(kernel)
add_subdirectory(moduleinit)
add_subdirectory(pci)
add_subdirectory(acpi)

