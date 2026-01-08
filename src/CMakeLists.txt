# ------------------------------------------
# Dump generated assembly to readable files.

if (DUMP_ASM)
    add_compile_options(-save-temps -fverbose-asm)
endif()

# ------------------------
# Our DSP utility library.

set(DSP_SOURCES dsp/dsp_tools.hpp)
add_library(DspTools ${DSP_SOURCES})
set_target_properties(DspTools PROPERTIES LINKER_LANGUAGE CXX)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# -----------------
# Add our programs.

# Examples.

add_executable(FirstExampleWithQ examples/first_example_w_q.cpp)
target_link_libraries(FirstExampleWithQ libqio)

add_executable(SimpleAlsaPlayer examples/simple_alsa_audio.cpp)
target_link_libraries(SimpleAlsaPlayer fmt ${ALSA_LIBRARIES})

add_executable(KfrReaderTest examples/kfr_reader_test.cpp)
target_link_libraries(KfrReaderTest fmt kfr kfr_io)

add_executable(QueueTest examples/queue_test.cpp)
target_link_libraries(QueueTest SPSCQueue)

add_executable(KfrTest examples/kfr_test.cpp)
target_link_libraries(KfrTest fmt kfr kfr_io DspTools)

# -------------
# Audio player.

# For ncurses version.
set(CURSES_NEED_NCURSES ON)
find_package(Curses)

if(CURSES_HAVE_NCURSES_H)
    # Our curses wrapper library.
    add_library(CursesConsole curses_console/curses_console.hpp curses_console/curses_console.cpp)
    target_include_directories(CursesConsole PUBLIC curses_console/)
    target_link_libraries(CursesConsole ${CURSES_LIBRARIES})

    set(AudioPlayer_sources
            audio_player/audio_player_main.cpp
            audio_player/lib/audio_player.hpp
            audio_player/lib/audio_player_app.hpp
            audio_player/lib/alsa_player.hpp
            audio_player/lib/alsa_player.cpp
            audio_player/lib/threadsafe_queue.hpp
            audio_player/lib/rt_queue.hpp
            audio_player/lib/filter.hpp
    )
    add_executable(AudioPlayer "${AudioPlayer_sources}")
    target_include_directories(AudioPlayer PRIVATE audio_player/)
    target_link_libraries(AudioPlayer fmt kfr kfr_io kfr_dft CursesConsole ${ALSA_LIBRARIES} SPSCQueue DspTools)
endif()


# ----------------------------------
# Add ALSA "official" test programs.

add_executable(AlsaPcmExample examples/alsa_official/pcm.c)
target_link_libraries(AlsaPcmExample ${ALSA_LIBRARIES} m)
