enable_testing()

add_executable(EmployeeTests test_employee.cpp)
target_include_directories(EmployeeTests PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(EmployeeTests PRIVATE GTest::gtest_main)
target_compile_options(EmployeeTests PRIVATE /W4)

include(GoogleTest)
gtest_discover_tests(EmployeeTests)
