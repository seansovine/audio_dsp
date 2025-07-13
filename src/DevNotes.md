# Audio Player Development Notes

## Running with Clang sanitizers

In `CMakeLists.txt` set

```cmake
set(ENABLE_SANITIZERS On)
```

then reconfigure and rebuild in debug mode.

The to get the output of the sanitizers run the program with

```shell
build/Debug/AudioPlayer 2>stderr.log
```

## TODO

+ Handle file load failure.
+ Implement playback for stereo files.
