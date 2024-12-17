cmake_minimum_required(VERSION 3.15...3.31)

project(AudioDSP
        VERSION 1.0
        LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED On)

# --------------------------
# Add third party libraries.

add_subdirectory(thirdparty/Q)

# -----------------
# Add our programs.

add_executable(FirstExampleWithQ src/examples/first_example_w_q.cpp)
target_link_libraries(FirstExampleWithQ libqio)
