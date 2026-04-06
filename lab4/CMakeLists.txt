cmake_minimum_required(VERSION 3.16)
project(Lab4_Sync CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# ---- Receiver ----
add_executable(Receiver src/Receiver.cpp)
target_compile_definitions(Receiver PRIVATE UNICODE _UNICODE)

# ---- Sender ----
add_executable(Sender src/Sender.cpp)
target_compile_definitions(Sender PRIVATE UNICODE _UNICODE)

# ---- Tests ----
add_executable(Lab4Tests tests/tests.cpp)
target_compile_definitions(Lab4Tests PRIVATE UNICODE _UNICODE)

enable_testing()
add_test(NAME Lab4Tests COMMAND Lab4Tests)
