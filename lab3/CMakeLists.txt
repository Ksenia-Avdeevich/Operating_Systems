cmake_minimum_required(VERSION 3.14)
project(Lab3_ThreadSync VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── Threads (pthreads on Linux) ───────────────────────────────────────────────
find_package(Threads REQUIRED)

# ─── Main executable ───────────────────────────────────────────────────────────
add_executable(lab3 src/main.cpp)
target_link_libraries(lab3 PRIVATE Threads::Threads)

# Warnings
target_compile_options(lab3 PRIVATE
    -Wall -Wextra -Wpedantic
)

# ─── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_lab3 tests/test_lab3.cpp)
target_link_libraries(test_lab3 PRIVATE Threads::Threads)
target_compile_options(test_lab3 PRIVATE
    -Wall -Wextra -Wpedantic
)

add_test(NAME UnitTests COMMAND test_lab3)
