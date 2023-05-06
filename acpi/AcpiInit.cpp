/*
   AcpiInit.cpp
   ACPI initizalization
   Copyright (c) 2023, Ilya Shamukov <ilya.shamukov@gmail.com>
   
   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option) 
   any later version.
   
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
   more details.
   
   You should have received a copy of the GNU General Public License along with 
   this program; if not, write to the Free Software Foundation, Inc., 59 Temple 
   Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "AcpiInit.h"

#define _AcpiModuleName "AcpiInit"

static void notifyHandler(ACPI_HANDLE, UINT32 value, void*)
{
    ACPI_INFO((AE_INFO, "Received a notify 0x%X", value));
}

static ACPI_STATUS installHandlers()
{
    const ACPI_STATUS status = AcpiInstallNotifyHandler(ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY, notifyHandler, nullptr);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While installing Notify handler"));
        return status;
    }

    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While installing an OpRegion handler"));
        return status;
    }

    return AE_OK;
}

ACPI_STATUS initializeFullAcpica()
{
    ACPI_STATUS status = AcpiInitializeSubsystem();
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPICA"));
        return status;
    }

    ACPI_INFO((AE_INFO, "Loading ACPI tables"));
    status = AcpiInitializeTables(nullptr, 16, FALSE);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing Table Manager"));
        return status;
    }

    status = AcpiLoadTables();
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While loading ACPI tables"));
        return status;
    }

    status = installHandlers();
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While installing handlers"));
        return status;
    }

    status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While enabling ACPICA"));
        return status;
    }

    status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE(status))
    {
        ACPI_EXCEPTION((AE_INFO, status, "While initializing ACPICA objects"));
        return status;
    }

    return AE_OK;
}
