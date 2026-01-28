# Not a proper Makefile; this is just used for development convenience.

format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: configure build run

configure:
	@/usr/local/bin/cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang \
		-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ --no-warn-unused-cli -S /home/sean/Code_projects/audio_dsp \
		-B /home/sean/Code_projects/audio_dsp/build/Release -G Ninja

build:
	@/usr/local/bin/cmake --build /home/sean/Code_projects/audio_dsp/build/Release --config Release --target all "-j 8" --

run:
	@build/Release/src/AudioPlayer
