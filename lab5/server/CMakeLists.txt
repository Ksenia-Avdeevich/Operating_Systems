add_executable(Server server.cpp)
target_include_directories(Server PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(Server PRIVATE /W4)
