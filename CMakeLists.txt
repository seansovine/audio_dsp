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

# KFR

add_subdirectory(thirdparty/kfr)

# ---------------------------
# Add to global include path.

include_directories(thirdparty/kfr/include)

# Provide C-string var for repo root.
#  - Borrowed from De Vries' LearnOpenGL repo.

configure_file(configuration/root_directory.h.in configuration/root_directory.h)
include_directories(${CMAKE_BINARY_DIR}/configuration)

# -----------------
# Add our programs.

# Examples.

add_executable(FirstExampleWithQ src/examples/first_example_w_q.cpp)
target_link_libraries(FirstExampleWithQ libqio)

add_executable(SimpleAlsaPlayer src/examples/simple_alsa_audio.cpp)
target_link_libraries(SimpleAlsaPlayer fmt ${ALSA_LIBRARIES})

add_executable(KfrReaderTest src/examples/kfr_reader_test.cpp)
target_link_libraries(KfrReaderTest fmt kfr kfr_io)

# Audio player.

# For ncurses version.
set(CURSES_NEED_NCURSES ON)
find_package(Curses)

if(CURSES_HAVE_NCURSES_H)
    # Our curses wrapper library.
    add_library(CursesConsole src/curses_console/curses_console.h src/curses_console/curses_console.cpp)
    target_include_directories(CursesConsole PUBLIC src/curses_console/)
    target_link_libraries(CursesConsole ${CURSES_LIBRARIES})

    set(AudioPlayer_sources
            src/audio_player/audio_player_main.cpp
            src/audio_player/lib/audio_player.h
            src/audio_player/lib/audio_player_app.h
            src/audio_player/lib/alsa_player.h
            src/audio_player/lib/alsa_player.cpp
            src/audio_player/lib/threadsafe_queue.h
    )
    add_executable(AudioPlayer "${AudioPlayer_sources}")
    target_link_libraries(AudioPlayer fmt kfr kfr_io CursesConsole ${ALSA_LIBRARIES})
endif()
