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
	${OBJECTDIR}/BootIo.o \
	${OBJECTDIR}/KernelLoader.o \
	${OBJECTDIR}/UefiVideo.o \
	${OBJECTDIR}/config.o \
	${OBJECTDIR}/conout.o \
	${OBJECTDIR}/cpu.o \
	${OBJECTDIR}/entry.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/memory.o \
	${OBJECTDIR}/new.o \
	${OBJECTDIR}/paging.o \
	${OBJECTDIR}/panic.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=-std=c++17 -masm=intel -fno-exceptions -fno-rtti -fno-threadsafe-statics
CXXFLAGS=-std=c++17 -masm=intel -fno-exceptions -fno-rtti -fno-threadsafe-statics

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bootx64.efi.exe

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bootx64.efi.exe: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/bootx64.efi ${OBJECTFILES} ${LDLIBSOPTIONS} -m64 -nostdlib -Wl,--pic-executable -Wl,--subsystem,10 -Wl,--dll -e _Z5entryPvP16EFI_SYSTEM_TABLE -s

${OBJECTDIR}/BootIo.o: BootIo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/BootIo.o BootIo.cpp

${OBJECTDIR}/KernelLoader.o: KernelLoader.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/KernelLoader.o KernelLoader.cpp

${OBJECTDIR}/UefiVideo.o: UefiVideo.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/UefiVideo.o UefiVideo.cpp

${OBJECTDIR}/config.o: config.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/config.o config.cpp

${OBJECTDIR}/conout.o: conout.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/conout.o conout.cpp

${OBJECTDIR}/cpu.o: cpu.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/cpu.o cpu.cpp

${OBJECTDIR}/entry.o: entry.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/entry.o entry.cpp

${OBJECTDIR}/main.o: main.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/memory.o: memory.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/memory.o memory.cpp

${OBJECTDIR}/new.o: new.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/new.o new.cpp

${OBJECTDIR}/paging.o: paging.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/paging.o paging.cpp

${OBJECTDIR}/panic.o: panic.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -Wall -I../include -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/panic.o panic.cpp

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
