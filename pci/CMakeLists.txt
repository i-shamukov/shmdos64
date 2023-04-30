set(HEADERS
    ${KERNEL_MODULE_INCLUDE_ROOT}/pci.h
    PciLegacy.h
    PciRoot.h
)

set(SOURCES
    main.cpp
    PciLegacy.cpp
    PciRoot.cpp
)

add_library(pci SHARED ${HEADERS} ${SOURCES})
kernel_module(pci)
target_link_libraries(pci PRIVATE kernel)
