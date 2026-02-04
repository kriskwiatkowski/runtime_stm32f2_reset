cmake_minimum_required(VERSION 3.10)
set(RUNNER_TARGET stm32f215_cw)
project(
    ${RUNNER_TARGET}_runtime
    VERSION 1.0.1
    LANGUAGES C ASM)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}")
set(CMAKE_INSTALL_INCLUDEDIR include/platform)

include(GNUInstallDirs)
include(cmake/postbuild)

string(APPEND CMAKE_C_FLAGS " ${C_CXX_FLAGS} ${EXTRA_C_FLAGS}")
string(APPEND CMAKE_ASM_FLAGS " ${EXTRA_ASM_FLAGS}")

# Removes quotes
separate_arguments(LOCAL_COMPILE_OPTIONS NATIVE_COMMAND "${EXTRA_COMPILE_OPTIONS}")
add_compile_options(${LOCAL_COMPILE_OPTIONS})

# Compiler specific flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS/startup_gnu/stm32f215ret6.ld)
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
    set(LINKER_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/src/CMSIS/startup_armcc/stm32f215ret6.sct)
else()
    message(ERROR "Unknown compiler")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    string(APPEND CMAKE_C_FLAGS " -O0")
endif()

# ---- Declare target library ----
add_library(${RUNNER_TARGET} STATIC
    src/printf.c
    src/stm32f2_hal_lowlevel.c
    src/stm32f2_hal.c
    src/platform.c
    src/rng.c
)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_sources(
        ${RUNNER_TARGET} PRIVATE
        src/CMSIS/startup_gnu/newlib_stubs.c
        src/CMSIS/startup_gnu/startup_stm32f2.S)
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
    target_sources(
        ${RUNNER_TARGET} PRIVATE
        src/CMSIS/startup_armcc/startup_stm32f217xx.S)
endif()

target_sources(
    ${RUNNER_TARGET}
    PUBLIC
    FILE_SET HEADERS
    FILES
        ${CMAKE_SOURCE_DIR}/inc/platform/printf.h
        ${CMAKE_SOURCE_DIR}/inc/platform/platform.h
        ${CMAKE_SOURCE_DIR}/inc/platform/utils.h
        ${CMAKE_SOURCE_DIR}/inc/platform/stack_usage.h
    BASE_DIRS
        ${CMAKE_SOURCE_DIR}/inc/platform)

target_include_directories(
    ${RUNNER_TARGET}
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/inc>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/CMSIS/core>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/CMSIS/device>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src/Legacy>)

target_compile_definitions(${RUNNER_TARGET}
    PRIVATE
    -DSTM32F215RETx -DSTM32F215xx -DSTM32F2 -DSTM32)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_options(${RUNNER_TARGET} PUBLIC "-T${LINKER_SCRIPT}")
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
    target_link_options(${RUNNER_TARGET} PUBLIC
        "SHELL:--scatter ${LINKER_SCRIPT}"
        --summary_stderr
        --map
        --load_addr_map_info
        --xref
        --callgraph
        --symbols
        "SHELL:--info summarysizes"
        "SHELL:--info sizes"
        "SHELL:--info totals"
        "SHELL:--info unused"
        "SHELL:--info veneers"
        "SHELL:--list $<TARGET_FILE:${RUNNER_TARGET}>.map")
endif()

target_link_directories(
    ${RUNNER_TARGET} INTERFACE
    $<INSTALL_INTERFACE:lib>)

include(GenerateExportHeader)
generate_export_header(
    ${RUNNER_TARGET}
    BASE_NAME ${RUNNER_TARGET}
    EXPORT_FILE_NAME export/${RUNNER_TARGET}/${RUNNER_TARGET}_export.h)

# ---- Hello world application ----
add_executable(
    hello
    app/hello.c)

target_compile_definitions(hello
    PRIVATE
    -DSTM32F215RETx -DSTM32F2 -DSTM32 -DDEBUG)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(
        hello PRIVATE
        -Wl,--whole-archive ${RUNNER_TARGET} -Wl,--no-whole-archive)
elseif(CMAKE_C_COMPILER_ID STREQUAL "ARMCC")
    target_link_libraries(hello PRIVATE ${RUNNER_TARGET})
endif()

target_postbuild_executable(hello)
include(cmake/install-rules.cmake)

# ---- CPack ----
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOW)
string(TOLOWER "${CMAKE_C_COMPILER_ID}" CMAKE_C_COMPILER_ID_LOWER)
set(CPACK_GENERATOR "TGZ")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "${RUNNER_TARGET}_${PROJECT_VERSION}_${CMAKE_C_COMPILER_ID_LOWER}_${CMAKE_BUILD_TYPE_LOW}")
include(CPack)
