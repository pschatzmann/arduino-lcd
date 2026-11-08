cmake_minimum_required(VERSION 3.14)
project(lcd_tests CXX)

# Desktop tests for the LCD library. The library is run against an emulated
# HD44780 display: the frames of the display are written as PNG files into
# the "output" directory of the build folder.
#
#   cmake -S test -B build/test
#   cmake --build build/test
#   ctest --test-dir build/test --output-on-failure

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LCD_TEST_SANITIZE "Build the tests with address and undefined behavior sanitizer" ON)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)

add_executable(run_tests ${TEST_SOURCES})
target_include_directories(run_tests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/mock
  ${CMAKE_CURRENT_SOURCE_DIR}/emulator
  ${CMAKE_CURRENT_SOURCE_DIR}/tests
  ${CMAKE_CURRENT_SOURCE_DIR}/../src)
target_compile_options(run_tests PRIVATE -g -O1 -Wall -Wextra)

if(LCD_TEST_SANITIZE)
  target_compile_options(run_tests PRIVATE -fsanitize=address,undefined)
  target_link_options(run_tests PRIVATE -fsanitize=address,undefined)
endif()

enable_testing()

# every test case is registered as an own CTest test
set(LCD_TEST_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/output)
set(LCD_DISCOVERED ${CMAKE_CURRENT_BINARY_DIR}/discovered_tests.cmake)
add_custom_command(TARGET run_tests POST_BUILD
  COMMAND ${CMAKE_COMMAND}
    -DTEST_EXE=$<TARGET_FILE:run_tests> -DOUT=${LCD_DISCOVERED}
    -DOUTPUT_DIR=${LCD_TEST_OUTPUT_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/DiscoverTests.cmake
  VERBATIM)
if(NOT EXISTS ${LCD_DISCOVERED})
  file(WRITE ${LCD_DISCOVERED} "")
endif()
set_directory_properties(PROPERTIES TEST_INCLUDE_FILES ${LCD_DISCOVERED})

# removes the images of the previous run
add_test(NAME lcd_clean_output
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${LCD_TEST_OUTPUT_DIR})
set_tests_properties(lcd_clean_output PROPERTIES FIXTURES_SETUP lcd_output)
