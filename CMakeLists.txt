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

# ------------------------
# Our DSP utility library.

set(DSP_SOURCES src/dsp/dsp_tools.hpp)
add_library(DspTools ${DSP_SOURCES})
set_target_properties(DspTools PROPERTIES LINKER_LANGUAGE CXX)

include_directories(${CMAKE_SOURCE_DIR}/src)

# -----------------
# Add our programs.

# Examples.

add_executable(FirstExampleWithQ src/examples/first_example_w_q.cpp)
target_link_libraries(FirstExampleWithQ libqio)

add_executable(SimpleAlsaPlayer src/examples/simple_alsa_audio.cpp)
target_link_libraries(SimpleAlsaPlayer fmt ${ALSA_LIBRARIES})

add_executable(KfrReaderTest src/examples/kfr_reader_test.cpp)
target_link_libraries(KfrReaderTest fmt kfr kfr_io)

add_executable(QueueTest src/examples/queue_test.cpp)
target_link_libraries(QueueTest SPSCQueue)

add_executable(KfrTest src/examples/kfr_test.cpp)
target_link_libraries(KfrTest fmt kfr kfr_io DspTools)

# -------------
# Audio player.

# For ncurses version.
set(CURSES_NEED_NCURSES ON)
find_package(Curses)

if(CURSES_HAVE_NCURSES_H)
    # Our curses wrapper library.
    add_library(CursesConsole src/curses_console/curses_console.hpp src/curses_console/curses_console.cpp)
    target_include_directories(CursesConsole PUBLIC src/curses_console/)
    target_link_libraries(CursesConsole ${CURSES_LIBRARIES})

    set(AudioPlayer_sources
            src/audio_player/audio_player_main.cpp
            src/audio_player/lib/audio_player.hpp
            src/audio_player/lib/audio_player_app.hpp
            src/audio_player/lib/alsa_player.hpp
            src/audio_player/lib/alsa_player.cpp
            src/audio_player/lib/threadsafe_queue.hpp
            src/audio_player/lib/rt_queue.hpp
    )
    add_executable(AudioPlayer "${AudioPlayer_sources}")
    target_include_directories(AudioPlayer PRIVATE src/audio_player/)
    target_link_libraries(AudioPlayer fmt kfr kfr_io kfr_dft CursesConsole ${ALSA_LIBRARIES} SPSCQueue DspTools)
endif()


# ----------------------------------
# Add ALSA "official" test programs.

add_executable(AlsaPcmExample src/examples/alsa_official/pcm.c)
target_link_libraries(AlsaPcmExample ${ALSA_LIBRARIES} m)
