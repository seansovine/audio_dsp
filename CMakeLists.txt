cmake_minimum_required(VERSION 3.15...3.31)

project(AudioDSP
        VERSION 1.0
        LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)

set(CMAKE_EXPORT_COMPILE_COMMANDS On)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)
set(CMAKE_LINKER_TYPE LLD)

# KFR wants this, but the CPU in one of our machines doesn't support it.
# TODO: Look at KFR build config to see if we can change AVX support.
# add_compile_options(-mavx512f)

# -----------------------------------------
# Dump generated assembly for object files.

set(DUMP_ASM On)

# ------------------------
# Maybe enable sanitizers.

set(ENABLE_SANITIZERS On)

if (ENABLE_SANITIZERS AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
    message("NOTE: Enabling clang sanitizers for this build.")

    # Enable AddressSanitizer and UndefinedBehaviorSanitizer
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined")

    # Optional: Add -fno-omit-frame-pointer for better stack traces
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
endif()

# --------------------------
# Add third party libraries.

# Joel de Guzman's Q library.

add_subdirectory(thirdparty/Q)

# fmt library.

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.2
        GIT_SUBMODULES_RECURSE ON
)

FetchContent_MakeAvailable(fmt)

# ALSA.

find_package(ALSA REQUIRED)

if (NOT ALSA_FOUND)
    message(FATAL_ERROR "ALSA library was not found; aborting build.")
endif()

# KFR

add_subdirectory(thirdparty/kfr)

# rigtorp/SPSCQueue

add_subdirectory(thirdparty/SPSCQueue)

# ---------------------------
# Add to global include path.

include_directories(thirdparty/kfr/include)

# Provide C-string var for repo root.
#  - Borrowed from De Vries' LearnOpenGL repo.

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

add_subdirectory(src)
