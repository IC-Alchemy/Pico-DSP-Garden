# CMake generated Testfile for 
# Source directory: Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests
# Build directory: Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/build-codex
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(rpdsp_tests "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/build-codex/Debug/rpdsp_tests.exe")
  set_tests_properties(rpdsp_tests PROPERTIES  _BACKTRACE_TRIPLES "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;22;add_test;Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(rpdsp_tests "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/build-codex/Release/rpdsp_tests.exe")
  set_tests_properties(rpdsp_tests PROPERTIES  _BACKTRACE_TRIPLES "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;22;add_test;Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(rpdsp_tests "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/build-codex/MinSizeRel/rpdsp_tests.exe")
  set_tests_properties(rpdsp_tests PROPERTIES  _BACKTRACE_TRIPLES "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;22;add_test;Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(rpdsp_tests "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/build-codex/RelWithDebInfo/rpdsp_tests.exe")
  set_tests_properties(rpdsp_tests PROPERTIES  _BACKTRACE_TRIPLES "Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;22;add_test;Z:/Codezzz/03_Music_Audio_MIDI/MusicCode/Pico-DSP-Garden/tests/CMakeLists.txt;0;")
else()
  add_test(rpdsp_tests NOT_AVAILABLE)
endif()
