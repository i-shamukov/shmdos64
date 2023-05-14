/*
   AcpiOsFunc.cpp
   ACPI OS API for SHM DOS64
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

//#pragma GCC diagnostic push 
//#pragma GCC diagnostic ignored "-Wunused-parameter"
extern "C"
{
#include <acpi.h>
}
//#pragma GCC diagnostic pop

#include <kclib.h>
#include <kchrono.h>
#include <ksem.h>
#include <kspin_lock.h>
#include <VirtualMemoryManager.h>
#include <IoResource.h>
#include <cpu.h>
#include <memory>
#include <kunordered_map.h>
#include <pci.h>
#include <AbstractDevice.h>
#include <AbstractDriver.h>
#include <ThreadPool.h>
#include <kthread.h>
#include <common_lib.h>
#include <kcondition_variable.h>
#include <kmutex.h>

#include <conout.h>
#include <vmem_utils.h>

static kunordered_map<uintptr_t, std::unique_ptr<AbstractDevice>> g_acpiDevs;
static const std::unique_ptr<IoResource> g_IoPortResource(makeIoPortResource(0, 0xFFFF));
static kcondition_variable g_asyncCallCv;
static kmutex g_asyncCallMutex;
static int g_asyncCallCount = 0;


static std::pair<IoResource*, uintptr_t> getMmio(uintptr_t address, UINT32 width)
{
    static kunordered_map<uintptr_t, std::unique_ptr<IoResource>> mmioMap;
    const uintptr_t alingAddr = address & ~PAGE_MASK;
    auto it = mmioMap.find(alingAddr);
    if (it != mmioMap.end())
        return std::make_pair(it->second.get(), address & PAGE_MASK);
    
    if (((address + (width / 8) - 1) & ~PAGE_MASK) == alingAddr)
    {
        const uintptr_t prevAlingAddr = address - PAGE_SIZE;
        it = mmioMap.find(prevAlingAddr);
        if (it != mmioMap.end())
            return std::make_pair(it->second.get(), PAGE_SIZE + (address & PAGE_MASK));
    }
    
    IoResource* mmio = makeMmioResource(alingAddr, PAGE_SIZE * 2);
    if (mmio == nullptr)
        return std::make_pair<IoResource*, uintptr_t>(nullptr, 0);
    
    mmioMap.emplace(alingAddr, mmio);
    return std::make_pair(mmio, address & PAGE_MASK);
}

static IoResource* getPciSpace(const ACPI_PCI_ID* pciId)
{
    static kunordered_map<uintptr_t, std::unique_ptr<IoResource>> pciMap;
    const uint64_t key = (pciId->Bus << 8) | ((pciId->Device & 0x1F) << 3) | (pciId->Function & 0x07);
    auto it = pciMap.find(key);
    if (it != pciMap.end())
        return it->second.get();
    
    PciAddress addr{pciId->Bus, pciId->Device, pciId->Function};
    IoResource* mmio = makePciSpaceIoResource(addr);
    if (mmio == nullptr)
        return nullptr;
    
    pciMap.emplace(key, mmio);
    return mmio;
}

static ACPI_STATUS genericRead(IoResource* mmio, uint64_t reg, UINT64* value, UINT32 width)
{
    if (mmio == nullptr)
        return AE_ERROR;

    switch (width)
    {
        case 8:
            *value = mmio->in8(reg);
            return AE_OK;

        case 16:
            *value = mmio->in16(reg);
            return AE_OK;

        case 32:
            *value = mmio->in32(reg);
            return AE_OK;

        case 64:
            *value = mmio->in64(reg);
            return AE_OK;

        default:
            return AE_ERROR;
    }
    return AE_OK; 
}

static ACPI_STATUS genericWrite(IoResource* mmio, uint64_t reg, UINT64 value, UINT32 width)
{
    if (mmio == nullptr)
        return AE_ERROR;

    switch (width)
    {
        case 8:
            mmio->out8(reg, (value & 0xFF));
            return AE_OK;

        case 16:
            mmio->out16(reg, (value & 0xFFFF));
            return AE_OK;

        case 32:
            mmio->out32(reg, (value & 0xFFFFFFFF));
            return AE_OK;

        case 64:
            mmio->out64(reg, value);
            return AE_OK;

        default:
            return AE_ERROR;
    }
    return AE_OK; 
}

#define ACPI_OS_DEBUG_CALL\
//    print(__func__);


extern "C"
{
    void* AcpiOsAllocate(ACPI_SIZE size)
    {
        ACPI_OS_DEBUG_CALL
        return (operator new[])(size);
    }

    void AcpiOsFree(void* ptr)
    {
        ACPI_OS_DEBUG_CALL
        return (operator delete[])(ptr);
    }

    ACPI_STATUS AcpiOsSignal(UINT32, void*)
    {
        ACPI_OS_DEBUG_CALL
        return AE_OK; 
    }

    UINT64 AcpiOsGetTimer()
    {
        ACPI_OS_DEBUG_CALL
        return getSystemClockNs100();
    }

    void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *format, ...)
    {
        ACPI_OS_DEBUG_CALL
        va_list args;
        va_start(args, format);
        kvprintf(format, args);
        va_end(args);
    }

    ACPI_STATUS AcpiOsCreateSemaphore(UINT32 max, UINT32 initial, ACPI_SEMAPHORE* handle)
    {
        ACPI_OS_DEBUG_CALL
        *handle = static_cast<ACPI_SEMAPHORE>(new ksem(initial, max));
        if (*handle == nullptr)
            return AE_ERROR;
        return AE_OK;
    }

    ACPI_STATUS AcpiOsTerminate()
    {
        ACPI_OS_DEBUG_CALL
        return AE_OK;
    }

    ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, UINT32 count)
    {
        //ACPI_OS_DEBUG_CALL
        uint32_t oldCount;
        static_cast<ksem*>(handle)->signal(count, &oldCount);
        return AE_OK;
    }

    ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, UINT32 count, UINT16 timeout)
    {
        //ACPI_OS_DEBUG_CALL
        return (static_cast<ksem*>(handle)->wait(count, ((timeout == 0xFFFF) ? ksem::WaitInfinite : TimePointFromMilliseconds(timeout))) ? AE_OK : AE_TIME);
    }

    ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle)
    {
        ACPI_OS_DEBUG_CALL
        delete  static_cast<ksem*>(handle);
        return AE_OK; 
    }

    ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER*, ACPI_PHYSICAL_ADDRESS* newAddress, UINT32* newTableLength)
    {
        ACPI_OS_DEBUG_CALL
        *newAddress = 0;
        *newTableLength = 0;
        return AE_OK;
    }

    ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER*, ACPI_TABLE_HEADER** newTable)
    {
        ACPI_OS_DEBUG_CALL
        *newTable = nullptr;
        return AE_OK;
    }

    void AcpiOsUnmapMemory(void* logicalAddress, ACPI_SIZE)
    {
        ACPI_OS_DEBUG_CALL
        if (!isRamMappingPtr(logicalAddress))
            VirtualMemoryManager::system().unmapMmio(logicalAddress);
    }

    void* AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS physicalAddress, ACPI_SIZE length)
    {
        ACPI_OS_DEBUG_CALL
        if (isPhysToVirtualConvertible(physicalAddress))
            return physToVirtualInt<void>(physicalAddress);

        return VirtualMemoryManager::system().mapMmio(physicalAddress, length, false);
    }

    ACPI_STATUS AcpiOsPredefinedOverride (const ACPI_PREDEFINED_NAMES*, ACPI_STRING* newValue)
    {
        ACPI_OS_DEBUG_CALL
        *newValue = nullptr;
        return AE_OK;
    }

    ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK* outHandle)
    {
        ACPI_OS_DEBUG_CALL
        *outHandle = static_cast<ACPI_SPINLOCK>(new kspin_lock());
        return AE_OK;
    }

    void AcpiOsDeleteLock(ACPI_HANDLE handle)
    {
        ACPI_OS_DEBUG_CALL
        delete static_cast<kspin_lock*>(handle);
    }

    ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle)
    {
        //ACPI_OS_DEBUG_CALL
        static_cast<kspin_lock*>(handle)->lock();
        return 0;
    }

    void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS)
    {
        //ACPI_OS_DEBUG_CALL
        static_cast<kspin_lock*>(handle)->unlock();
    }

    ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS address, UINT64* value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        auto [mmio, offset] = getMmio(address, width);
        return genericRead(mmio, offset, value, width);
    }

    ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS address, UINT64 value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        auto [mmio, offset] = getMmio(address, width);
        return genericWrite(mmio, offset, value, width);
    }

    ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64* value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        return genericRead(getPciSpace(pciId), reg, value, width);
    }

    ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID* pciId, UINT32 reg, UINT64 value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        return genericWrite(getPciSpace(pciId), reg, value, width);
    }

    ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 irq, ACPI_OSD_HANDLER handler, void* context)
    {
        ACPI_OS_DEBUG_CALL
        const uintptr_t key = (reinterpret_cast<uintptr_t>(handler) & CPU_PHYSICAL_ADDRESS_MASK) | (static_cast<uintptr_t>(irq) << 56);
        if (g_acpiDevs.find(key) != g_acpiDevs.end())
            return AE_ERROR;

        class AcpiDevice : public AbstractDevice
        {
        public:
            AcpiDevice(ACPI_OSD_HANDLER handler, void* context)
                : AbstractDevice(DeviceClass::System, L"ACPI SCI", AbstractDevice::root(), AbstractDriver::kernel())
                , m_handler(handler)
                , m_context(context)
            {
            }

            bool installInterruptHandler(unsigned int irq)
            {
                return AbstractDevice::installInterruptHandler(irq);
            }
        
        private:
            bool interruptHandler() override
            {
                return (m_handler(m_context) == ACPI_INTERRUPT_HANDLED);
            }
        
        private:
            ACPI_OSD_HANDLER m_handler;
            void* m_context;
        };

        std::unique_ptr<AcpiDevice> dev = std::make_unique<AcpiDevice>(handler, context);
        if (!dev->installInterruptHandler(irq))
            return AE_ERROR;
        
        g_acpiDevs.emplace(key, std::move(dev));
        return AE_OK;
    }

    ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 irq, ACPI_OSD_HANDLER handler)
    {
        ACPI_OS_DEBUG_CALL
        const uintptr_t key = (reinterpret_cast<uintptr_t>(handler) & CPU_PHYSICAL_ADDRESS_MASK) | (static_cast<uintptr_t>(irq) << 56);
        auto it = g_acpiDevs.find(key);
        if (it == g_acpiDevs.end())
            return AE_ERROR;

        g_acpiDevs.erase(it);
        return AE_OK;
    }

    ACPI_STATUS AcpiOsInitialize()
    {
        ACPI_OS_DEBUG_CALL
        return AE_OK;
    }

    void AcpiOsVprintf(const char *format, va_list args)
    {
        ACPI_OS_DEBUG_CALL
        kvprintf(format, args);
    }

    ACPI_THREAD_ID AcpiOsGetThreadId()
    {
        //ACPI_OS_DEBUG_CALL
        return kthis_thread::get_id();
    }

    ACPI_STATUS AcpiOsReadPort (ACPI_IO_ADDRESS port, UINT32* value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        UINT64 value64;
        const ACPI_STATUS result = genericRead(g_IoPortResource.get(), port, &value64, width);
        *value = value64 & 0xFFFFFFFF;
        return result;
    }

    ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS port, UINT32 value, UINT32 width)
    {
        ACPI_OS_DEBUG_CALL
        return genericWrite(g_IoPortResource.get(), port, value, width);
    }

    void AcpiOsStall(UINT32 microseconds)
    {
        ACPI_OS_DEBUG_CALL
        sleepUs(microseconds);
    }

    void AcpiOsSleep(UINT64 milliseconds)
    {
        ACPI_OS_DEBUG_CALL
        sleepMs(milliseconds);
    }
    
    ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE, ACPI_OSD_EXEC_CALLBACK function, void* context)
    {
        ACPI_OS_DEBUG_CALL
        ThreadPool::system().run([context, function]{
            {
                klock_guard lock(g_asyncCallMutex);
                ++g_asyncCallCount;
            }
            function(context);
            {
                klock_guard lock(g_asyncCallMutex);
                --g_asyncCallCount;
            }
            g_asyncCallCv.notify_all();
        });
        return AE_OK;
    }

    void AcpiOsWaitEventsComplete()
    {
        ACPI_OS_DEBUG_CALL
        kunique_lock lock(g_asyncCallMutex);
        while (g_asyncCallCount > 0)
            g_asyncCallCv.wait(lock);
    }

    ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
    {
        return acpiRsdpPhys();
    }
}