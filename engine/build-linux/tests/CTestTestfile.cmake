# CMake generated Testfile for 
# Source directory: /home/z/my-project/work/core_recovery/tests
# Build directory: /home/z/my-project/work/core_recovery/build-linux/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
include("/home/z/my-project/work/core_recovery/build-linux/tests/cr_tests_e3b0c442_include.cmake")
include("/home/z/my-project/work/core_recovery/build-linux/tests/cr_ui_tests_e3b0c442_include.cmake")
add_test("cr_ui_abi_check" "/home/z/my-project/work/core_recovery/build-linux/tests/cr_ui_abi_check" "/home/z/my-project/work/core_recovery/build-linux/tests/cr_ui_abi_check.img")
set_tests_properties("cr_ui_abi_check" PROPERTIES  _BACKTRACE_TRIPLES "/home/z/my-project/work/core_recovery/tests/CMakeLists.txt;134;add_test;/home/z/my-project/work/core_recovery/tests/CMakeLists.txt;0;")
add_test("win_syntax_check" "/home/z/my-project/work/core_recovery/tests/../tools/win_syntax_check.sh" "/home/z/my-project/work/core_recovery")
set_tests_properties("win_syntax_check" PROPERTIES  _BACKTRACE_TRIPLES "/home/z/my-project/work/core_recovery/tests/CMakeLists.txt;167;add_test;/home/z/my-project/work/core_recovery/tests/CMakeLists.txt;0;")
