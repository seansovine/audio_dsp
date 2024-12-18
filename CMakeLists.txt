cmake_minimum_required(VERSION 3.15...3.31)

project(AudioDSP
        VERSION 1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED On)

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

# -----------------
# Add our programs.

add_executable(FirstExampleWithQ src/examples/first_example_w_q.cpp)
target_link_libraries(FirstExampleWithQ libqio)

add_executable(SimpleAlsaPlayer src/examples/simple_alsa_audio.cpp)
target_link_libraries(SimpleAlsaPlayer fmt ${ALSA_LIBRARIES})
