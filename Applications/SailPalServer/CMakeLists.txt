cmake_minimum_required(VERSION 3.16)

project(SailPalServer)

set(SOURCES
	src/main.cpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	list(APPEND SOURCES
		src/platform/linux/CxaThreadAtexitCompat.cpp
	)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
	NexusCore
)

target_compile_definitions(${PROJECT_NAME}
	PRIVATE
		QT_NO_PRIVATE_MODULE_WARNING
)

find_package(Qt6 REQUIRED COMPONENTS Core Network HttpServer)
target_link_libraries(${PROJECT_NAME} PRIVATE
	Qt6::Core
	Qt6::Network
	Qt6::HttpServer
)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION bin
	COMPONENT Runtime
)