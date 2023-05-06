# SHM DOS64
This is operating system kernel project 

## Why is this needed?
This project is my hobby. In this project I am implementing some of my ideas for basic concepts of OS kernel and working with hardware. 

## Which of interesting things has been implemented now
- fully 64-bit kernel (x86-64)
- modern hardware support 
- SMP and multitasking support
- three-level memory allocator (physical, virtual and local heap) with algorithmic complexity O(1)
- EFI bootloader
- code is written in C++17 with minimum of low-level fragments
- support kernel modules in PE DLL format
- implemented standard synchronization primitives: mutex, queued spinlock, condition variable, semaphore, event, etc
- implemented analogues of some STL classes and functions

## repository guide
- /binary - latest kernel binaries
- /include - kernel modules shared headers
- /kernel - kernel main module sources
- /lib - shared libs
- /uefi - EFI bootloader
- all other folders contains kernel module sources

## How to fast start
- format flash drive in FAT32
- copy files from /binary to drive root
- disable "Secure Boot" in UEFI BIOS and select flash drive as main boot device
- reload and enjoy result :)
- similar actions can be done for virtual machine with support EFI boot

## Build instruction
For build all projects need CMake with MinGW-w64 compiler. No special environment, package or library settings are required.

## Plans for future
- PCI Device enumerator
- kernel asynchronous IO manager (async read-write files, etc)
- support some parts of USB, for example, xHCI
- support FAT (r/w) and NTFS (r/o)
- user-mode layer implementation
- implementation of simple user-mode GUI and command shell
- support build in unix-based OS
- support alternate loading ELF executable modules mechanism
- support MSI
- support multiple IO APIC's

