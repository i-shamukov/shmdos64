#pragma once
#include <common_types.h>

#define IMAGE_DOS_SIGNATURE 0x5A4D
#define IMAGE_NT_SIGNATURE 0x00004550

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES 16
#define IMAGE_SIZEOF_SHORT_NAME 8

#define IMAGE_SCN_TYPE_NO_PAD 8
#define IMAGE_SCN_CNT_CODE 32
#define IMAGE_SCN_CNT_INITIALIZED_DATA 64
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 128
#define IMAGE_SCN_LNK_OTHER 256
#define IMAGE_SCN_LNK_INFO 512
#define IMAGE_SCN_LNK_REMOVE 2048
#define IMAGE_SCN_LNK_COMDAT 4096
#define IMAGE_SCN_MEM_FARDATA 0x8000
#define IMAGE_SCN_MEM_PURGEABLE 0x20000
#define IMAGE_SCN_MEM_16BIT 0x20000
#define IMAGE_SCN_MEM_LOCKED  0x40000
#define IMAGE_SCN_MEM_PRELOAD 0x80000
#define IMAGE_SCN_ALIGN_1BYTES 0x100000
#define IMAGE_SCN_ALIGN_2BYTES 0x200000
#define IMAGE_SCN_ALIGN_4BYTES 0x300000
#define IMAGE_SCN_ALIGN_8BYTES 0x400000
#define IMAGE_SCN_ALIGN_16BYTES 0x500000
#define IMAGE_SCN_ALIGN_32BYTES 0x600000
#define IMAGE_SCN_ALIGN_64BYTES 0x700000
#define IMAGE_SCN_LNK_NRELOC_OVFL 0x1000000
#define IMAGE_SCN_MEM_DISCARDABLE 0x2000000
#define IMAGE_SCN_MEM_NOT_CACHED 0x4000000
#define IMAGE_SCN_MEM_NOT_PAGED 0x8000000
#define IMAGE_SCN_MEM_SHARED 0x10000000
#define IMAGE_SCN_MEM_EXECUTE 0x20000000
#define IMAGE_SCN_MEM_READ 0x40000000
#define IMAGE_SCN_MEM_WRITE 0x80000000

#define IMAGE_DIRECTORY_ENTRY_EXPORT	0
#define IMAGE_DIRECTORY_ENTRY_IMPORT	1
#define IMAGE_DIRECTORY_ENTRY_RESOURCE	2
#define IMAGE_DIRECTORY_ENTRY_EXCEPTION	3
#define IMAGE_DIRECTORY_ENTRY_SECURITY	4
#define IMAGE_DIRECTORY_ENTRY_BASERELOC	5
#define IMAGE_DIRECTORY_ENTRY_DEBUG	6
#define IMAGE_DIRECTORY_ENTRY_COPYRIGHT	7
#define IMAGE_DIRECTORY_ENTRY_GLOBALPTR	8
#define IMAGE_DIRECTORY_ENTRY_TLS	9
#define IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG	10
#define IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT	11
#define IMAGE_DIRECTORY_ENTRY_IAT	12
#define IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT 14

#define IMAGE_FILE_MACHINE_AMD64  	0x8664

#define IMAGE_FILE_DLL 0x2000

#pragma pack(push, 1)
typedef struct _IMAGE_DOS_HEADER {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;
} IMAGE_DOS_HEADER,*PIMAGE_DOS_HEADER;

typedef struct _IMAGE_FILE_HEADER {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
} IMAGE_FILE_HEADER, *PIMAGE_FILE_HEADER;

typedef struct _IMAGE_DATA_DIRECTORY {
    uint32_t VirtualAddress;
    uint32_t Size;
} IMAGE_DATA_DIRECTORY,*PIMAGE_DATA_DIRECTORY;

typedef struct _IMAGE_OPTIONAL_HEADER 
{
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint64_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint64_t SizeOfStackReserve;
    uint64_t SizeOfStackCommit;
    uint64_t SizeOfHeapReserve;
    uint64_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} IMAGE_OPTIONAL_HEADER,*PIMAGE_OPTIONAL_HEADER;

typedef struct _IMAGE_NT_HEADERS {
    uint32_t Signature;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_OPTIONAL_HEADER OptionalHeader;
} IMAGE_NT_HEADERS,*PIMAGE_NT_HEADERS;

typedef struct _IMAGE_SECTION_HEADER {
    uint8_t Name[IMAGE_SIZEOF_SHORT_NAME];
    union {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
} IMAGE_SECTION_HEADER,*PIMAGE_SECTION_HEADER;

typedef struct _IMAGE_BASE_RELOCATION {
    uint32_t VirtualAddress;
    uint32_t SizeOfBlock;
} IMAGE_BASE_RELOCATION,*PIMAGE_BASE_RELOCATION;

typedef struct _IMAGE_IMPORT_DESCRIPTOR {
    union {
            uint32_t Characteristics;
            uint32_t OriginalFirstThunk;
     };
     uint32_t TimeDateStamp;
     uint32_t ForwarderChain;
     uint32_t Name;
     uint32_t FirstThunk;
} IMAGE_IMPORT_DESCRIPTOR;

typedef struct _IMAGE_EXPORT_DIRECTORY 
{
    uint32_t   Characteristics;
    uint32_t   TimeDateStamp;
    uint16_t    MajorVersion;
    uint16_t    MinorVersion;
    uint32_t   Name;
    uint32_t   Base;
    uint32_t   NumberOfFunctions;
    uint32_t   NumberOfNames;
    uint32_t   AddressOfFunctions;     
    uint32_t   AddressOfNames;         
    uint32_t   AddressOfNameOrdinals;  
} IMAGE_EXPORT_DIRECTORY, *PIMAGE_EXPORT_DIRECTORY;
#pragma pack(pop)