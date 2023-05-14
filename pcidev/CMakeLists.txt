set(HEADERS
    PciBus.h
    ${KERNEL_MODULE_INCLUDE_ROOT}/PciDevice.h
)

set(SOURCES
    main.cpp
    PciBus.cpp
    PciDevice.cpp
)

add_library(pcidev SHARED ${HEADERS} ${SOURCES})
kernel_module(pcidev ON)
target_link_libraries(pcidev PRIVATE kernel pci)
target_compile_definitions(pcidev PRIVATE PCIDEVICE_EXPORT)