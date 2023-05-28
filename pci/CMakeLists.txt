set(HEADERS
    ${KERNEL_MODULE_INCLUDE_ROOT}/pci.h
    PciLegacy.h
    PciRoot.h
)

set(SOURCES
    main.cpp
    PciLegacy.cpp
    PciRoot.cpp
    IrqRouting.cpp
)

add_library(pci SHARED ${HEADERS} ${SOURCES})
kernel_module(pci ON)
target_link_libraries(pci PRIVATE kernel)
target_compile_definitions(pci PRIVATE PCI_EXPORT)