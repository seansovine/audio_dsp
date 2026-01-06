format:
	@find src/ -regex '.*\.\(cpp\|hpp\|c\|h\|cc\|hh\|cxx\|hxx\)' -exec clang-format -style=file -i {} \;

.PHONY: configure
configure:
	@/usr/local/bin/cmake -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang \
		-DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ --no-warn-unused-cli -S /home/sean/Code_projects/audio_dsp \
		-B /home/sean/Code_projects/audio_dsp/build/Release -G Ninja

.PHONY: build
build:
	@/usr/local/bin/cmake --build /home/sean/Code_projects/audio_dsp/build/Release --config Release --target all "-j 8" --

.PHONY: run
run:
	@build/Release/AudioPlayer
