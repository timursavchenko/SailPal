cmake_minimum_required(VERSION 3.16)

project(SailPal)

set(GUI_ENABLED OFF CACHE BOOL "Enable GUI support" FORCE)
set(NEXUSCORE_APP_NAME "SailPalServer" CACHE STRING "NexusCore application name" FORCE)

# Silence Qt private module warnings triggered by transitive Qt modules.
set(QT_NO_PRIVATE_MODULE_WARNING ON CACHE BOOL "Disable Qt private module warnings")

if(CMAKE_SYSROOT)
	set(SAILPAL_TARGET_PREFIX "${CMAKE_SYSROOT}/usr")
	list(PREPEND CMAKE_PREFIX_PATH "${SAILPAL_TARGET_PREFIX}")
	if(NOT DEFINED CMAKE_FIND_ROOT_PATH_MODE_PACKAGE OR CMAKE_FIND_ROOT_PATH_MODE_PACKAGE STREQUAL "")
		set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
	endif()
	set(Qt6_DIR "${SAILPAL_TARGET_PREFIX}/lib/cmake/Qt6" CACHE PATH "Target Qt6 package directory" FORCE)
endif()

if(NEXUS_QT_HOST_PATH)
	set(QT_HOST_PATH "${NEXUS_QT_HOST_PATH}")
	set(QT_HOST_PATH "${NEXUS_QT_HOST_PATH}" CACHE PATH "Qt host path" FORCE)
endif()
if(NEXUS_QT_HOST_PATH_CMAKE_DIR)
	set(QT_HOST_PATH_CMAKE_DIR "${NEXUS_QT_HOST_PATH_CMAKE_DIR}")
	set(QT_HOST_PATH_CMAKE_DIR "${NEXUS_QT_HOST_PATH_CMAKE_DIR}" CACHE PATH "Qt host CMake package directory" FORCE)
endif()

add_subdirectory(NexusCore)
add_subdirectory(Applications/SailPalServer)