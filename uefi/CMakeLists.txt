set(HEADERS
    BootIo.h
    config.h
    efi.h
    KernelLoader.h
    memory.h
    paging.h
    panic.h
    UefiVideo.h
    ${KERNEL_MODULE_INCLUDE_ROOT}/conout.h
    ${KERNEL_MODULE_INCLUDE_ROOT}/cpu.h
)

set(SOURCES
    BootIo.cpp
    config.cpp
    KernelLoader.cpp
    conout.cpp
    cpu.cpp
    entry.cpp
    main.cpp
    memory.cpp
    new.cpp
    paging.cpp
    panic.cpp
    UefiVideo.cpp
)

add_executable(uefi ${HEADERS} ${SOURCES})
kernel_efi_module(uefi)
set_target_properties(uefi PROPERTIES OUTPUT_NAME bootx64 SUFFIX .efi)
