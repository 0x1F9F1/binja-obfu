cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

set(CMAKE_VS_INCLUDE_INSTALL_TO_DEFAULT_BUILD ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(WIN32)
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()

project(binja_obfu CXX)

if(MSVC)
    add_compile_options(/MP)
endif()

if(WIN32)
    set(BINJA_DIR "C:\\Program Files\\Vector35\\BinaryNinja"
        CACHE PATH "Binary Ninja installation directory")
    set(BINJA_BIN_DIR "${BINJA_DIR}")
    set(BINJA_PLUGINS_DIR "$ENV{APPDATA}/Binary Ninja/plugins"
        CACHE PATH "Binary Ninja user plugins directory")
elseif(APPLE)
    set(BINJA_DIR "/Applications/Binary Ninja.app"
        CACHE PATH "Binary Ninja installation directory")
    set(BINJA_BIN_DIR "${BINJA_DIR}/Contents/MacOS")
    set(BINJA_PLUGINS_DIR "$ENV{HOME}/Library/Application Support/Binary Ninja/plugins"
        CACHE PATH "Binary Ninja user plugins directory")
else()
    set(BINJA_DIR "$ENV{HOME}/binaryninja"
        CACHE PATH "Binary Ninja installation directory")
    set(BINJA_BIN_DIR "${BINJA_DIR}")
    set(BINJA_PLUGINS_DIR "$ENV{HOME}/.binaryninja/plugins"
        CACHE PATH "Binary Ninja user plugins directory")
endif()

find_library(BINJA_CORE_LIBRARY
    NAMES binaryninjacore libbinaryninjacore.so.1
    PATHS ${BINJA_BIN_DIR})

message("Binary Ninja Core at " ${BINJA_CORE_LIBRARY})

add_subdirectory(vendor EXCLUDE_FROM_ALL)

add_library(${PROJECT_NAME} SHARED
    src/DataBufferAdapter.cpp
    src/main.cpp
    src/MLIL.cpp
    src/MLIL_SSA.cpp
    src/ObfuArchitectureHook.cpp
    src/ObfuPasses.cpp
    src/ObjectDestructionNotification.cpp
    src/PatchBuilder.cpp
    include/BackgroundTaskThread.h
    include/BinaryNinja.h
    include/BinaryViewAssociatedDataStore.h
    include/DataBufferAdapter.h
    include/MLIL.h
    include/MLIL_SSA.h
    include/ObfuArchitectureHook.h
    include/ObfuPasses.h
    include/ObjectDestructionNotification.h
    include/PatchBuilder.h)

find_library(BINJA_CORE_LIBRARY binaryninjacore
    HINTS ${BINJA_BIN_DIR})

target_include_directories(${PROJECT_NAME} SYSTEM
    PRIVATE include
    PRIVATE vendor/binaryninja-api)

target_link_libraries(${PROJECT_NAME}
    ${BINJA_CORE_LIBRARY} binaryninjaapi fmt bitsery)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin
)

if(WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME
        DESTINATION ${BINJA_PLUGINS_DIR})

    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
        DESTINATION ${BINJA_PLUGINS_DIR} OPTIONAL)
else()
    install(TARGETS ${PROJECT_NAME} LIBRARY
        DESTINATION ${BINJA_PLUGINS_DIR})
endif()
