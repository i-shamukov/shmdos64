set(HEADERS
    AcpiInit.h
    PciIrqRouting.h
)

set(SOURCES
    main.cpp
    AcpiOsFunc.cpp
    AcpiInit.cpp
    PciIrqRouting.cpp
)

add_library(acpi SHARED ${HEADERS} ${SOURCES})
kernel_module(acpi OFF)
target_link_directories(acpi PRIVATE ${EXTLIBS}/acpica/lib/$<CONFIG>)
target_include_directories(acpi SYSTEM PRIVATE ${EXTLIBS}/acpica/include)
target_link_libraries(acpi PRIVATE acpica kernel pci)
target_compile_definitions(acpi PRIVATE _SHMDOS64)