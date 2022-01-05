#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=MinGW_x64-Windows
CND_DLIB_EXT=dll
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/AbstractDevice.o \
	${OBJECTDIR}/AbstractTimer.o \
	${OBJECTDIR}/AcpiTables.o \
	${OBJECTDIR}/BootVideo.o \
	${OBJECTDIR}/EventObject.o \
	${OBJECTDIR}/ExceptionHandlers.o \
	${OBJECTDIR}/ExternalInterrupts.o \
	${OBJECTDIR}/Heap.o \
	${OBJECTDIR}/Hpet.o \
	${OBJECTDIR}/InterruptQueue.o \
	${OBJECTDIR}/InterruptQueuePool.o \
	${OBJECTDIR}/IoResourceimpl.o \
	${OBJECTDIR}/LocalApic.o \
	${OBJECTDIR}/PeLoader.o \
	${OBJECTDIR}/Process.o \
	${OBJECTDIR}/Semaphore.o \
	${OBJECTDIR}/SmpBoot.o \
	${OBJECTDIR}/SpinLock.o \
	${OBJECTDIR}/TaskManager.o \
	${OBJECTDIR}/ThreadPool.o \
	${OBJECTDIR}/VirtualMemoryManager.o \
	${OBJECTDIR}/bootlib.o \
	${OBJECTDIR}/common_lib.o \
	${OBJECTDIR}/cpu.o \
	${OBJECTDIR}/entry.o \
	${OBJECTDIR}/gdt.o \
	${OBJECTDIR}/idt.o \
	${OBJECTDIR}/kcondition_variable.o \
	${OBJECTDIR}/kmutex.o \
	${OBJECTDIR}/kthread.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/new.o \
	${OBJECTDIR}/paging.o \
	${OBJECTDIR}/panic.o \
	${OBJECTDIR}/phmem.o \
	${OBJECTDIR}/smp.o \
	${OBJECTDIR}/tests.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++17 -masm=intel -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wno-builtin-declaration-mismatch -mcx16 -mno-red-zone -mno-sse
CXXFLAGS=-std=c++17 -masm=intel -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wno-builtin-declaration-mismatch -mcx16 -mno-red-zone -mno-sse

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/kernel.exe

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/kernel.exe: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/kernel ${OBJECTFILES} ${LDLIBSOPTIONS} -m64 -nostdlib -Wl,--pic-executable -Wl,--out-implib,..\lib\libkernel.a -e _Z5entryP12KernelParams -s

${OBJECTDIR}/AbstractDevice.o: AbstractDevice.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/AbstractDevice.o AbstractDevice.cpp

${OBJECTDIR}/AbstractTimer.o: AbstractTimer.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/AbstractTimer.o AbstractTimer.cpp

${OBJECTDIR}/AcpiTables.o: AcpiTables.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/AcpiTables.o AcpiTables.cpp

${OBJECTDIR}/BootVideo.o: BootVideo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/BootVideo.o BootVideo.cpp

${OBJECTDIR}/EventObject.o: EventObject.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/EventObject.o EventObject.cpp

${OBJECTDIR}/ExceptionHandlers.o: ExceptionHandlers.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ExceptionHandlers.o ExceptionHandlers.cpp

${OBJECTDIR}/ExternalInterrupts.o: ExternalInterrupts.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ExternalInterrupts.o ExternalInterrupts.cpp

${OBJECTDIR}/Heap.o: Heap.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Heap.o Heap.cpp

${OBJECTDIR}/Hpet.o: Hpet.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Hpet.o Hpet.cpp

${OBJECTDIR}/InterruptQueue.o: InterruptQueue.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/InterruptQueue.o InterruptQueue.cpp

${OBJECTDIR}/InterruptQueuePool.o: InterruptQueuePool.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/InterruptQueuePool.o InterruptQueuePool.cpp

${OBJECTDIR}/IoResourceimpl.o: IoResourceimpl.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/IoResourceimpl.o IoResourceimpl.cpp

${OBJECTDIR}/LocalApic.o: LocalApic.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/LocalApic.o LocalApic.cpp

${OBJECTDIR}/PeLoader.o: PeLoader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/PeLoader.o PeLoader.cpp

${OBJECTDIR}/Process.o: Process.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Process.o Process.cpp

${OBJECTDIR}/Semaphore.o: Semaphore.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/Semaphore.o Semaphore.cpp

${OBJECTDIR}/SmpBoot.o: SmpBoot.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SmpBoot.o SmpBoot.cpp

${OBJECTDIR}/SpinLock.o: SpinLock.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/SpinLock.o SpinLock.cpp

${OBJECTDIR}/TaskManager.o: TaskManager.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/TaskManager.o TaskManager.cpp

${OBJECTDIR}/ThreadPool.o: ThreadPool.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/ThreadPool.o ThreadPool.cpp

${OBJECTDIR}/VirtualMemoryManager.o: VirtualMemoryManager.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/VirtualMemoryManager.o VirtualMemoryManager.cpp

${OBJECTDIR}/bootlib.o: bootlib.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/bootlib.o bootlib.cpp

${OBJECTDIR}/common_lib.o: common_lib.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/common_lib.o common_lib.cpp

${OBJECTDIR}/cpu.o: cpu.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cpu.o cpu.cpp

${OBJECTDIR}/entry.o: entry.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/entry.o entry.cpp

${OBJECTDIR}/gdt.o: gdt.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/gdt.o gdt.cpp

${OBJECTDIR}/idt.o: idt.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/idt.o idt.cpp

${OBJECTDIR}/kcondition_variable.o: kcondition_variable.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/kcondition_variable.o kcondition_variable.cpp

${OBJECTDIR}/kmutex.o: kmutex.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/kmutex.o kmutex.cpp

${OBJECTDIR}/kthread.o: kthread.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/kthread.o kthread.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/new.o: new.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/new.o new.cpp

${OBJECTDIR}/paging.o: paging.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/paging.o paging.cpp

${OBJECTDIR}/panic.o: panic.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/panic.o panic.cpp

${OBJECTDIR}/phmem.o: phmem.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/phmem.o phmem.cpp

${OBJECTDIR}/smp.o: smp.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/smp.o smp.cpp

${OBJECTDIR}/tests.o: tests.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -DKERNEL_EXPORT -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/tests.o tests.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
