#ifndef __ACSHMDOS64_H__
#define __ACSHMDOS64_H__

#include <stdint.h>

#define ACPI_USE_STANDARD_HEADERS

#define ACPI_USE_LOCAL_CACHE
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_DO_WHILE_0
#define ACPI_FLUSH_CPU_CACHE() asm volatile("wbinvd");

#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t

#define ACPI_MUTEX_TYPE  ACPI_OSL_MUTEX

#include "acgcc.h"

#endif
