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
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/moduleinit.o


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
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ../lib/libmoduleinit.a

../lib/libmoduleinit.a: ${OBJECTFILES}
	${MKDIR} -p ../lib
	${RM} ../lib/libmoduleinit.a
	${AR} -rv ../lib/libmoduleinit.a ${OBJECTFILES} 
	$(RANLIB) ../lib/libmoduleinit.a

${OBJECTDIR}/moduleinit.o: moduleinit.cpp
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/moduleinit.o moduleinit.cpp

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
