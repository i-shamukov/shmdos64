/** 
  This include file must contain things defined in the UEFI 2.7 specification.
  If a code construct is defined in the UEFI 2.7 specification it must be included
  by this include file.
Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

typedef void VOID;
typedef unsigned long long UINTN;
typedef unsigned long long UINT64;
typedef unsigned long UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long INT32;
typedef short INT16;
typedef UINTN EFI_STATUS;
typedef UINTN EFI_TPL;
typedef VOID* EFI_HANDLE;
typedef wchar_t CHAR16;
typedef UINT8 BOOLEAN;
typedef VOID* EFI_EVENT;

typedef UINT64 EFI_PHYSICAL_ADDRESS;
typedef UINT64 EFI_VIRTUAL_ADDRESS;


#define EFI_SUCCESS               0

#define EFIAPI
#define IN
#define OUT
#define OPTIONAL

typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS(EFIAPI *EFI_INPUT_RESET) (
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    IN BOOLEAN ExtendedVerification
    );

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY) (
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    OUT EFI_INPUT_KEY *Key
    );

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET Reset;
    EFI_INPUT_READ_KEY ReadKeyStroke;
    EFI_EVENT WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN ExtendedVerification);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN const CHAR16 *String);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_TEST_STRING) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16 *String
);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN ModeNumber,
    OUT UINTN *Columns,
    OUT UINTN *Rows);

typedef EFI_STATUS (*EFIAPI EFI_TEXT_SET_MODE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN ModeNumber);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN Attribute);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_CLEAR_SCREEN) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_SET_CURSOR_POSITION) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN Column,
    IN UINTN Row);

typedef EFI_STATUS(EFIAPI *EFI_TEXT_ENABLE_CURSOR) (
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN Visible);

typedef struct {
    INT32 MaxMode;
    // current settings
    INT32 Mode;
    INT32 Attribute;
    INT32 CursorColumn;
    INT32 CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET Reset;
    EFI_TEXT_STRING OutputString;
    EFI_TEXT_TEST_STRING TestString;
    EFI_TEXT_QUERY_MODE QueryMode;
    EFI_TEXT_SET_MODE SetMode;
    EFI_TEXT_SET_ATTRIBUTE SetAttribute;
    EFI_TEXT_CLEAR_SCREEN ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;


typedef struct {
    UINT16 Year; // 1900 – 9999
    UINT8 Month; // 1 – 12
    UINT8 Day; // 1 – 31
    UINT8 Hour; // 0 – 23
    UINT8 Minute; // 0 – 59
    UINT8 Second; // 0 – 59
    UINT8 Pad1;
    UINT32 Nanosecond; // 0 – 999,999,999
    INT16 TimeZone; // -1440 to 1440 or 2047
    UINT8 Daylight;
    UINT8 Pad2;
} EFI_TIME;

typedef struct {
    UINT32 Resolution;
    UINT32 Accuracy;
    BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;

typedef
EFI_STATUS
(EFIAPI *EFI_GET_TIME)(
  OUT  EFI_TIME                    *Time,
  OUT  EFI_TIME_CAPABILITIES       *Capabilities OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SET_TIME)(
  IN  EFI_TIME                     *Time
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_WAKEUP_TIME)(
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    *Time
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SET_WAKEUP_TIME)(
  IN  BOOLEAN                      Enable,
  IN  EFI_TIME                     *Time   OPTIONAL
  );


typedef struct {
  ///
  /// Type of the memory region.
  /// Type EFI_MEMORY_TYPE is defined in the
  /// AllocatePages() function description.
  ///
  UINT32                Type;
  ///
  /// Physical address of the first byte in the memory region. PhysicalStart must be
  /// aligned on a 4 KiB boundary, and must not be above 0xfffffffffffff000. Type
  /// EFI_PHYSICAL_ADDRESS is defined in the AllocatePages() function description
  ///
  EFI_PHYSICAL_ADDRESS  PhysicalStart;
  ///
  /// Virtual address of the first byte in the memory region.
  /// VirtualStart must be aligned on a 4 KiB boundary,
  /// and must not be above 0xfffffffffffff000.
  ///
  EFI_VIRTUAL_ADDRESS   VirtualStart;
  ///
  /// NumberOfPagesNumber of 4 KiB pages in the memory region.
  /// NumberOfPages must not be 0, and must not be any value
  /// that would represent a memory page with a start address,
  /// either physical or virtual, above 0xfffffffffffff000.
  ///
  UINT64                NumberOfPages;
  ///
  /// Attributes of the memory region that describe the bit mask of capabilities
  /// for that memory region, and not necessarily the current settings for that
  /// memory region.
  ///
  UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP)(
  IN  UINTN                        MemoryMapSize,
  IN  UINTN                        DescriptorSize,
  IN  UINT32                       DescriptorVersion,
  IN  EFI_MEMORY_DESCRIPTOR        *VirtualMap
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CONVERT_POINTER)(
  IN     UINTN                      DebugDisposition,
  IN OUT VOID                       **Address
  );

typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8 Data4[8];
} EFI_GUID;

typedef
EFI_STATUS
(EFIAPI *EFI_GET_VARIABLE)(
  IN     CHAR16                      *VariableName,
  IN     EFI_GUID                    *VendorGuid,
  OUT    UINT32                      *Attributes,    OPTIONAL
  IN OUT UINTN                       *DataSize,
  OUT    VOID                        *Data           OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME)(
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SET_VARIABLE)(
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT)(
  OUT UINT32                  *HighCount
  );


typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef
VOID
(EFIAPI *EFI_RESET_SYSTEM)(
  IN EFI_RESET_TYPE           ResetType,
  IN EFI_STATUS               ResetStatus,
  IN UINTN                    DataSize,
  IN VOID                     *ResetData OPTIONAL
  );

typedef struct {
    EFI_GUID CapsuleGuid;
    UINT32 HeaderSize;
    UINT32 Flags;
    UINT32 CapsuleImageSize;
} EFI_CAPSULE_HEADER;

typedef
EFI_STATUS
(EFIAPI *EFI_UPDATE_CAPSULE)(
  IN EFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN UINTN                  CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS   ScatterGatherList   OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES)(
  IN  EFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN  UINTN                  CapsuleCount,
  OUT UINT64                 *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE         *ResetType
  );

typedef
EFI_STATUS
(EFIAPI *EFI_QUERY_VARIABLE_INFO)(
  IN  UINT32            Attributes,
  OUT UINT64            *MaximumVariableStorageSize,
  OUT UINT64            *RemainingVariableStorageSize,
  OUT UINT64            *MaximumVariableSize
  );

typedef struct {
    EFI_TABLE_HEADER Hdr;
    //
    // Time Services
    //
    EFI_GET_TIME GetTime;
    EFI_SET_TIME SetTime;
    EFI_GET_WAKEUP_TIME GetWakeupTime;
    EFI_SET_WAKEUP_TIME SetWakeupTime;
    //
    // Virtual Memory Services
    //
    EFI_SET_VIRTUAL_ADDRESS_MAP SetVirtualAddressMap;
    EFI_CONVERT_POINTER ConvertPointer;
    //
    // Variable Services
    //
    EFI_GET_VARIABLE GetVariable;
    EFI_GET_NEXT_VARIABLE_NAME GetNextVariableName;
    EFI_SET_VARIABLE SetVariable;
    //
    // Miscellaneous Services
    //
    EFI_GET_NEXT_HIGH_MONO_COUNT GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM ResetSystem;
    //
    // UEFI 2.0 Capsule Services
    //
    EFI_UPDATE_CAPSULE UpdateCapsule;
    EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;
    //
    // Miscellaneous UEFI 2.0 Service
    //
    EFI_QUERY_VARIABLE_INFO QueryVariableInfo;
} EFI_RUNTIME_SERVICES;


typedef
EFI_TPL
(EFIAPI *EFI_RAISE_TPL)(
  IN EFI_TPL      NewTpl
  );

typedef
VOID
(EFIAPI *EFI_RESTORE_TPL)(
  IN EFI_TPL      OldTpl
  );

typedef enum {
  ///
  /// Allocate any available range of pages that satisfies the request.
  ///
  AllocateAnyPages,
  ///
  /// Allocate any available range of pages whose uppermost address is less than
  /// or equal to a specified maximum address.
  ///
  AllocateMaxAddress,
  ///
  /// Allocate pages at a specified address.
  ///
  AllocateAddress,
  ///
  /// Maximum enumeration value that may be used for bounds checking.
  ///
  MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES)(
  IN     EFI_ALLOCATE_TYPE            Type,
  IN     EFI_MEMORY_TYPE              MemoryType,
  IN     UINTN                        Pages,
  IN OUT EFI_PHYSICAL_ADDRESS         *Memory
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES)(
  IN  EFI_PHYSICAL_ADDRESS         Memory,
  IN  UINTN                        Pages
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP)(
  IN OUT UINTN                       *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR       *MemoryMap,
  OUT    UINTN                       *MapKey,
  OUT    UINTN                       *DescriptorSize,
  OUT    UINT32                      *DescriptorVersion
  );

typedef
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_POOL)(
  IN  EFI_MEMORY_TYPE              PoolType,
  IN  UINTN                        Size,
  OUT VOID                         **Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_FREE_POOL)(
  IN  VOID                         *Buffer
  );

typedef
VOID
(EFIAPI *EFI_EVENT_NOTIFY) (
    IN EFI_EVENT Event,
    IN VOID *Context
    );

typedef
EFI_STATUS
(EFIAPI *EFI_CREATE_EVENT)(
  IN  UINT32                       Type,
  IN  EFI_TPL                      NotifyTpl,
  IN  EFI_EVENT_NOTIFY             NotifyFunction,
  IN  VOID                         *NotifyContext,
  OUT EFI_EVENT                    *Event
  );

typedef enum {
TimerCancel,
TimerPeriodic,
TimerRelative
} EFI_TIMER_DELAY;

typedef
EFI_STATUS
(EFIAPI *EFI_SET_TIMER)(
  IN  EFI_EVENT                Event,
  IN  EFI_TIMER_DELAY          Type,
  IN  UINT64                   TriggerTime
  );

typedef
EFI_STATUS
(EFIAPI *EFI_WAIT_FOR_EVENT)(
  IN  UINTN                    NumberOfEvents,
  IN  EFI_EVENT                *Event,
  OUT UINTN                    *Index
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SIGNAL_EVENT)(
  IN  EFI_EVENT                Event
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_EVENT)(
  IN EFI_EVENT                Event
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CHECK_EVENT)(
  IN EFI_EVENT                Event
  );

typedef enum {
  ///
  /// Indicates that the supplied protocol interface is supplied in native form.
  ///
  EFI_NATIVE_INTERFACE
} EFI_INTERFACE_TYPE;


typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE)(
  IN OUT EFI_HANDLE               *Handle,
  IN     EFI_GUID                 *Protocol,
  IN     EFI_INTERFACE_TYPE       InterfaceType,
  IN     VOID                     *Interface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE)(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN VOID                     *OldInterface,
  IN VOID                     *NewInterface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE)(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN VOID                     *Interface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_HANDLE_PROTOCOL)(
  IN  EFI_HANDLE               Handle,
  IN  EFI_GUID                 *Protocol,
  OUT VOID                     **Interface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY)(
  IN  EFI_GUID                 *Protocol,
  IN  EFI_EVENT                Event,
  OUT VOID                     **Registration
  );


typedef enum {
  ///
  /// Retrieve all the handles in the handle database.
  ///
  AllHandles,
  ///
  /// Retrieve the next handle fron a RegisterProtocolNotify() event.
  ///
  ByRegisterNotify,
  ///
  /// Retrieve the set of handles from the handle database that support a
  /// specified protocol.
  ///
  ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE)(
  IN     EFI_LOCATE_SEARCH_TYPE   SearchType,
  IN     EFI_GUID                 *Protocol,    OPTIONAL
  IN     VOID                     *SearchKey,   OPTIONAL
  IN OUT UINTN                    *BufferSize,
  OUT    EFI_HANDLE               *Buffer
  );

typedef struct _EFI_DEVICE_PATH_PROTOCOL {
UINT8 Type;
UINT8 SubType;
UINT8 Length[2];
} EFI_DEVICE_PATH_PROTOCOL;

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_DEVICE_PATH)(
  IN     EFI_GUID                         *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath,
  OUT    EFI_HANDLE                       *Device
  );

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE)(
  IN EFI_GUID                 *Guid,
  IN VOID                     *Table
  );

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_LOAD)(
  IN  BOOLEAN                      BootPolicy,
  IN  EFI_HANDLE                   ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath,
  IN  VOID                         *SourceBuffer OPTIONAL,
  IN  UINTN                        SourceSize,
  OUT EFI_HANDLE                   *ImageHandle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_START)(
  IN  EFI_HANDLE                  ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData    OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_EXIT)(
  IN  EFI_HANDLE                   ImageHandle,
  IN  EFI_STATUS                   ExitStatus,
  IN  UINTN                        ExitDataSize,
  IN  CHAR16                       *ExitData     OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_UNLOAD)(
  IN  EFI_HANDLE                   ImageHandle
  );

typedef
EFI_STATUS
(EFIAPI *EFI_EXIT_BOOT_SERVICES)(
  IN  EFI_HANDLE                   ImageHandle,
  IN  UINTN                        MapKey
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT)(
  OUT UINT64                  *Count
  );

typedef
EFI_STATUS
(EFIAPI *EFI_STALL)(
  IN  UINTN                    Microseconds
  );

typedef
EFI_STATUS
(EFIAPI *EFI_SET_WATCHDOG_TIMER)(
  IN UINTN                    Timeout,
  IN UINT64                   WatchdogCode,
  IN UINTN                    DataSize,
  IN CHAR16                   *WatchdogData OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CONNECT_CONTROLLER)(
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    *DriverImageHandle,   OPTIONAL
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath, OPTIONAL
  IN  BOOLEAN                       Recursive
  );

typedef
EFI_STATUS
(EFIAPI *EFI_DISCONNECT_CONTROLLER)(
  IN  EFI_HANDLE                     ControllerHandle,
  IN  EFI_HANDLE                     DriverImageHandle, OPTIONAL
  IN  EFI_HANDLE                     ChildHandle        OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL)(
  IN  EFI_HANDLE                Handle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface, OPTIONAL
  IN  EFI_HANDLE                AgentHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL)(
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 *Protocol,
  IN EFI_HANDLE               AgentHandle,
  IN EFI_HANDLE               ControllerHandle
  );

typedef struct {
  EFI_HANDLE  AgentHandle;
  EFI_HANDLE  ControllerHandle;
  UINT32      Attributes;
  UINT32      OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;


typedef
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION)(
  IN  EFI_HANDLE                          Handle,
  IN  EFI_GUID                            *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                               *EntryCount
  );

typedef
EFI_STATUS
(EFIAPI *EFI_PROTOCOLS_PER_HANDLE)(
  IN  EFI_HANDLE      Handle,
  OUT EFI_GUID        ***ProtocolBuffer,
  OUT UINTN           *ProtocolBufferCount
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
  IN     EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN     EFI_GUID                     *Protocol,      OPTIONAL
  IN     VOID                         *SearchKey,     OPTIONAL
  IN OUT UINTN                        *NoHandles,
  OUT    EFI_HANDLE                   **Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL)(
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration, OPTIONAL
  OUT VOID      **Interface
  );

typedef
EFI_STATUS
(EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
  IN OUT EFI_HANDLE           *Handle,
  ...
  );

typedef
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES)(
  IN EFI_HANDLE           Handle,
  ...
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CALCULATE_CRC32)(
  IN  VOID                              *Data,
  IN  UINTN                             DataSize,
  OUT UINT32                            *Crc32
  );

typedef
VOID
(EFIAPI *EFI_COPY_MEM)(
  IN VOID     *Destination,
  IN VOID     *Source,
  IN UINTN    Length
  );

typedef
VOID
(EFIAPI *EFI_SET_MEM)(
  IN VOID     *Buffer,
  IN UINTN    Size,
  IN UINT8    Value
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CREATE_EVENT_EX)(
  IN       UINT32                 Type,
  IN       EFI_TPL                NotifyTpl,
  IN       EFI_EVENT_NOTIFY       NotifyFunction OPTIONAL,
  IN       VOID                   *NotifyContext OPTIONAL,
  IN       EFI_GUID               *EventGroup    OPTIONAL,
  OUT      EFI_EVENT              *Event
  );

typedef struct {
  ///
  /// The table header for the EFI Boot Services Table.
  ///
  EFI_TABLE_HEADER                Hdr;

  //
  // Task Priority Services
  //
  EFI_RAISE_TPL                   RaiseTPL;
  EFI_RESTORE_TPL                 RestoreTPL;

  //
  // Memory Services
  //
  EFI_ALLOCATE_PAGES              AllocatePages;
  EFI_FREE_PAGES                  FreePages;
  EFI_GET_MEMORY_MAP              GetMemoryMap;
  EFI_ALLOCATE_POOL               AllocatePool;
  EFI_FREE_POOL                   FreePool;

  //
  // Event & Timer Services
  //
  EFI_CREATE_EVENT                  CreateEvent;
  EFI_SET_TIMER                     SetTimer;
  EFI_WAIT_FOR_EVENT                WaitForEvent;
  EFI_SIGNAL_EVENT                  SignalEvent;
  EFI_CLOSE_EVENT                   CloseEvent;
  EFI_CHECK_EVENT                   CheckEvent;

  //
  // Protocol Handler Services
  //
  EFI_INSTALL_PROTOCOL_INTERFACE    InstallProtocolInterface;
  EFI_REINSTALL_PROTOCOL_INTERFACE  ReinstallProtocolInterface;
  EFI_UNINSTALL_PROTOCOL_INTERFACE  UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL               HandleProtocol;
  VOID                              *Reserved;
  EFI_REGISTER_PROTOCOL_NOTIFY      RegisterProtocolNotify;
  EFI_LOCATE_HANDLE                 LocateHandle;
  EFI_LOCATE_DEVICE_PATH            LocateDevicePath;
  EFI_INSTALL_CONFIGURATION_TABLE   InstallConfigurationTable;

  //
  // Image Services
  //
  EFI_IMAGE_LOAD                    LoadImage;
  EFI_IMAGE_START                   StartImage;
  EFI_EXIT                          Exit;
  EFI_IMAGE_UNLOAD                  UnloadImage;
  EFI_EXIT_BOOT_SERVICES            ExitBootServices;

  //
  // Miscellaneous Services
  //
  EFI_GET_NEXT_MONOTONIC_COUNT      GetNextMonotonicCount;
  EFI_STALL                         Stall;
  EFI_SET_WATCHDOG_TIMER            SetWatchdogTimer;

  //
  // DriverSupport Services
  //
  EFI_CONNECT_CONTROLLER            ConnectController;
  EFI_DISCONNECT_CONTROLLER         DisconnectController;

  //
  // Open and Close Protocol Services
  //
  EFI_OPEN_PROTOCOL                 OpenProtocol;
  EFI_CLOSE_PROTOCOL                CloseProtocol;
  EFI_OPEN_PROTOCOL_INFORMATION     OpenProtocolInformation;

  //
  // Library Services
  //
  EFI_PROTOCOLS_PER_HANDLE          ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER          LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL               LocateProtocol;
  EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES    InstallMultipleProtocolInterfaces;
  EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES  UninstallMultipleProtocolInterfaces;

  //
  // 32-bit CRC Services
  //
  EFI_CALCULATE_CRC32               CalculateCrc32;

  //
  // Miscellaneous Services
  //
  EFI_COPY_MEM                      CopyMem;
  EFI_SET_MEM                       SetMem;
  EFI_CREATE_EVENT_EX               CreateEventEx;
} EFI_BOOT_SERVICES;


typedef struct {
  ///
  /// The 128-bit GUID value that uniquely identifies the system configuration table.
  ///
  EFI_GUID                          VendorGuid;
  ///
  /// A pointer to the table associated with VendorGuid.
  ///
  VOID                              *VendorTable;
} EFI_CONFIGURATION_TABLE;

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249
#define EFI_2_60_SYSTEM_TABLE_REVISION ((2<<16) | (60))
#define EFI_2_50_SYSTEM_TABLE_REVISION ((2<<16) | (50))
#define EFI_2_40_SYSTEM_TABLE_REVISION ((2<<16) | (40))
#define EFI_2_31_SYSTEM_TABLE_REVISION ((2<<16) | (31))
#define EFI_2_30_SYSTEM_TABLE_REVISION ((2<<16) | (30))
#define EFI_2_20_SYSTEM_TABLE_REVISION ((2<<16) | (20))
#define EFI_2_10_SYSTEM_TABLE_REVISION ((2<<16) | (10))
#define EFI_2_00_SYSTEM_TABLE_REVISION ((2<<16) | (00))
#define EFI_1_10_SYSTEM_TABLE_REVISION ((1<<16) | (10))
#define EFI_1_02_SYSTEM_TABLE_REVISION ((1<<16) | (02))
#define EFI_SPECIFICATION_VERSION EFI_SYSTEM_TABLE_REVISION
#define EFI_SYSTEM_TABLE_REVISION EFI_2_60_SYSTEM_TABLE_REVISION

typedef struct {
    EFI_TABLE_HEADER Hdr;
    CHAR16 *FirmwareVendor;
    UINT32 FirmwareRevision;
    EFI_HANDLE ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
    EFI_HANDLE ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
    EFI_HANDLE StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
    EFI_RUNTIME_SERVICES *RuntimeServices;
    EFI_BOOT_SERVICES *BootServices;
    UINTN NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042a9de,0x23dc,0x4a38,\
    {0x96,0xfb,0x7a,0xde,0xd0,0x80,0x51,0x6a}}

typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    UINT32 Version;
    UINT32 HorizontalResolution;
    UINT32 VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    EFI_PIXEL_BITMASK PixelInformation;
    UINT32 PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info);

typedef EFI_STATUS(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber
);

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer, OPTIONAL
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX,
    IN UINTN SourceY,
    IN UINTN DestinationX,
    IN UINTN DestinationY,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Delta OPTIONAL);

typedef struct {
    UINT32 MaxMode;
    UINT32 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
    UINTN SizeOfInfo;
    EFI_PHYSICAL_ADDRESS FrameBufferBase;
    UINTN FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

#define EFI_LOADED_IMAGE_PROTOCOL_GUID\
    {0x5B1B31A1,0x9562,0x11d2,\
    {0x8E,0x3F,0x00,0xA0,0xC9,0x69,0x72,0x3B}}  

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    {0x0964e5b22,0x6459,0x11d2,\
    {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}

typedef struct {
    UINT32 Revision;
    EFI_HANDLE ParentHandle;
    EFI_SYSTEM_TABLE *SystemTable;
    // Source location of the image
    EFI_HANDLE DeviceHandle;
    EFI_DEVICE_PATH_PROTOCOL *FilePath;
    VOID *Reserved;
    // Image’s load options
    UINT32 LoadOptionsSize;
    VOID *LoadOptions;
    // Location where image was loaded
    VOID *ImageBase;
    UINT64 ImageSize;
    EFI_MEMORY_TYPE ImageCodeType;
    EFI_MEMORY_TYPE ImageDataType;
    EFI_IMAGE_UNLOAD Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

//*******************************************************
// Open Modes
//*******************************************************
#define EFI_FILE_MODE_READ 0x0000000000000001
#define EFI_FILE_MODE_WRITE 0x0000000000000002
#define EFI_FILE_MODE_CREATE 0x8000000000000000
//*******************************************************
// File Attributes
//*******************************************************
#define EFI_FILE_READ_ONLY 0x0000000000000001
#define EFI_FILE_HIDDEN 0x0000000000000002
#define EFI_FILE_SYSTEM 0x0000000000000004
#define EFI_FILE_RESERVED 0x0000000000000008
#define EFI_FILE_DIRECTORY 0x0000000000000010
#define EFI_FILE_ARCHIVE 0x0000000000000020
#define EFI_FILE_VALID_ATTR 0x0000000000000037

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN) (
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    const CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes);

typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE) (
    EFI_FILE_PROTOCOL *This);

typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE) (EFI_FILE_PROTOCOL *This);

typedef EFI_STATUS(EFIAPI *EFI_FILE_READ) (
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE) (
    EFI_FILE_PROTOCOL *This,
    UINTN *BufferSize,
    VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION) (
    EFI_FILE_PROTOCOL *This,
    UINT64 Position);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION) (
    EFI_FILE_PROTOCOL *This,
    UINT64 *Position);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO) (
    EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType,
    UINTN *BufferSize,
    VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO) (
    EFI_FILE_PROTOCOL *This,
    EFI_GUID *InformationType,
    UINTN BufferSize,
    VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH) (
    EFI_FILE_PROTOCOL *This);

typedef struct {
    EFI_EVENT Event;
    EFI_STATUS Status;
    UINTN BufferSize;
    VOID *Buffer;
} EFI_FILE_IO_TOKEN;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN_EX) (
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_PROTOCOL **NewHandle,
    CHAR16 *FileName,
    UINT64 OpenMode,
    UINT64 Attributes,
    EFI_FILE_IO_TOKEN *Token);

typedef EFI_STATUS (EFIAPI *EFI_FILE_READ_EX) (
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_IO_TOKEN *Token);

typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE_EX) (
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_IO_TOKEN *Token);

typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH_EX) (
    EFI_FILE_PROTOCOL *This,
    EFI_FILE_IO_TOKEN *Token);

typedef struct _EFI_FILE_PROTOCOL {
    UINT64 Revision;
    EFI_FILE_OPEN Open;
    EFI_FILE_CLOSE Close;
    EFI_FILE_DELETE Delete;
    EFI_FILE_READ Read;
    EFI_FILE_WRITE Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO GetInfo;
    EFI_FILE_SET_INFO SetInfo;
    EFI_FILE_FLUSH Flush;
    EFI_FILE_OPEN_EX OpenEx; // Added for revision 2
    EFI_FILE_READ_EX ReadEx; // Added for revision 2
    EFI_FILE_WRITE_EX WriteEx; // Added for revision 2
    EFI_FILE_FLUSH_EX FlushEx; // Added for revision 2
} EFI_FILE_PROTOCOL;

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME) (
    IN EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    OUT EFI_FILE_PROTOCOL **Root);

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64 Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

#define EFI_MP_SERVICES_PROTOCOL_GUID \
  {0x3fdda605, 0xa76e, 0x4f46, \
  {0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0x3d, 0x08}}

typedef struct _EFI_MP_SERVICES_PROTOCOL EFI_MP_SERVICES_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    OUT UINTN* NumberOfProcessors,
    OUT UINTN* NumberOfEnabledProcessors);

typedef struct 
{
    UINT32 Package;  
    UINT32 Core;  
    UINT32 Thread;
} EFI_CPU_PHYSICAL_LOCATION;

typedef struct {
    UINT64 ProcessorId; 
    UINT32 StatusFlag;
    EFI_CPU_PHYSICAL_LOCATION Location;
} EFI_PROCESSOR_INFORMATION;

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_GET_PROCESSOR_INFO) (
    IN  EFI_MP_SERVICES_PROTOCOL* This,
    IN  UINTN ProcessorNumber, 
    OUT EFI_PROCESSOR_INFORMATION* ProcessorInfoBuffer);

typedef VOID (EFIAPI* EFI_AP_PROCEDURE) (IN VOID *ProcedureArgument);

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_STARTUP_ALL_APS) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    IN EFI_AP_PROCEDURE Procedure,
    IN BOOLEAN SingleThread,
    IN EFI_EVENT WaitEvent OPTIONAL,
    IN UINTN TimeoutInMicroSeconds,
    IN VOID* ProcedureArgument OPTIONAL,
    OUT UINTN** FailedCpuList  OPTIONAL);

typedef EFI_STATUS (EFIAPI* EFI_MP_SERVICES_STARTUP_THIS_AP) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    IN EFI_AP_PROCEDURE Procedure,
    IN UINTN ProcessorNumber,
    IN  EFI_EVENT WaitEvent OPTIONAL,
    IN UINTN TimeoutInMicroseconds,
    IN VOID *ProcedureArgument OPTIONAL,
    OUT BOOLEAN *Finished OPTIONAL);

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_SWITCH_BSP) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    IN UINTN ProcessorNumber,  
    IN BOOLEAN EnableOldBSP);

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_ENABLEDISABLEAP) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    IN UINTN ProcessorNumber,
    IN BOOLEAN EnableAP,
    IN UINT32 *HealthFlag OPTIONAL);

typedef EFI_STATUS (EFIAPI *EFI_MP_SERVICES_WHOAMI) (
    IN EFI_MP_SERVICES_PROTOCOL* This,
    OUT UINTN *ProcessorNumber
);

typedef struct _EFI_MP_SERVICES_PROTOCOL { 
    EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS GetNumberOfProcessors; 
    EFI_MP_SERVICES_GET_PROCESSOR_INFO GetProcessorInfo; 
    EFI_MP_SERVICES_STARTUP_ALL_APS StartupAllAPs; 
    EFI_MP_SERVICES_STARTUP_THIS_AP StartupThisAP; 
    EFI_MP_SERVICES_SWITCH_BSP SwitchBSP; 
    EFI_MP_SERVICES_ENABLEDISABLEAP EnableDisableAP; 
    EFI_MP_SERVICES_WHOAMI WhoAmI;
} EFI_MP_SERVICES_PROTOCOL;

#define EFI_EDID_ACTIVE_PROTOCOL_GUID \
    {0xbd8c1056, 0x9f36, 0x44ec, \
    {0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86 }}

typedef struct {
    UINT32 SizeOfEdid;
    UINT8* Edid;
} EFI_EDID_ACTIVE_PROTOCOL;

const EFI_SYSTEM_TABLE* getSystemTable();
EFI_HANDLE getImageHandle();


