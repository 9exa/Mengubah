cmake_minimum_required(VERSION 3.15)
project(Mengubah VERSION 0.0.7)

# set(CMAKE_DEBUG_POSTFIX d)

# clangd needs to config to do debugging
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_library(mengu_compiler_flags INTERFACE)
target_compile_features(mengu_compiler_flags INTERFACE cxx_std_20)

# add compiler warning flags just when building this project via
# the BUILD_INTERFACE genex
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(mengu_compiler_flags INTERFACE
  "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wshadow;-Wformat=2;-Wunused;-g;-O3>>"
  "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
)

set(NANOGUI_BUILD_EXAMPLES OFF)
set(NANOGUI_BUILD_SHARED OFF)
set(NANOGUI_BUILD_PYTHON OFF)
set(NANOGUI_BUILD_EXAMPLES OFF)
set(NANOGUI_INSTALL OFF)
add_subdirectory(nanogui)


set(NANO_INCLUDE_DIR nanogui/include)
set(NANO_LIB_DIR "nanogui")
set(NANO_LIB "nanogui")


set(MINIAUDIO_INCLUE_DIR miniaudio)

set(MenguPitchy_SOURCE_DIR src)


set (TOP_SRC 
    "${MenguPitchy_SOURCE_DIR}/miniaudioimpl.c"
)

file(GLOB_RECURSE NESTED_SRC 
    "${MenguPitchy_SOURCE_DIR}/*/*.cpp" 
    #"${MenguPitchy_SOURCE_DIR}/*.h" 
    "${MenguPitchy_SOURCE_DIR}/*/*.c" 
)
set(ALL_SRC ${TOP_SRC} ${NESTED_SRC})


set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/arch/")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")


include_directories(PUBLIC ${NANO_INCLUDE_DIR} ${MINIAUDIO_INCLUE_DIR} ${MenguPitchy_SOURCE_DIR})
link_directories(PUBLIC ${NANO_LIB_DIR})

## Apps
add_executable(MenguPitchy)
target_link_libraries(MenguPitchy PUBLIC ${NANO_LIB} mengu_compiler_flags)
target_sources(MenguPitchy PRIVATE ${ALL_SRC} ${MenguPitchy_SOURCE_DIR}/mengupitchy.cpp)

add_executable(MenguStretchy)
target_link_libraries(MenguStretchy PUBLIC ${NANO_LIB} mengu_compiler_flags)
target_sources(MenguStretchy PRIVATE ${ALL_SRC} ${MenguPitchy_SOURCE_DIR}/mengustretchy.cpp)

add_executable(MenguVoice)
target_link_libraries(MenguVoice PUBLIC ${NANO_LIB} mengu_compiler_flags)
target_sources(MenguVoice PRIVATE ${ALL_SRC} ${MenguPitchy_SOURCE_DIR}/RealTimeChanger.cpp)


## Plugins


set(LV2_DEFAULT_DIR "${PROJECT_BINARY_DIR}/mengubah.lv2")
option(MENGU_LV2 "Whether or not to build the LV2" ON)

if (MENGU_LV2)
    add_library(Mengubah_lv2 MODULE)
    set_target_properties(Mengubah_lv2 PROPERTIES PREFIX "")
    set_target_properties(Mengubah_lv2 PROPERTIES OUTPUT_NAME "mengubah")
    set_target_properties(Mengubah_lv2 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${LV2_DEFAULT_DIR})
    target_link_libraries(Mengubah_lv2 PUBLIC ${NANO_LIB} mengu_compiler_flags)
    target_sources(Mengubah_lv2 PRIVATE ${ALL_SRC} ${MenguPitchy_SOURCE_DIR}/mengubah_lv2.cpp)
    configure_file(mengubah.ttl.in "${LV2_DEFAULT_DIR}/mengubah.ttl")
    configure_file(manifest.ttl.in "${LV2_DEFAULT_DIR}/manifest.ttl")
endif()

#Test executables
option(MENGU_BUILD_TESTS "Test executables that show correctness of dsp functions" OFF)
set (TEST_DIR "tests")

if(MENGU_BUILD_TESTS)
    add_executable(pvreconstruction ${ALL_SRC} ${TEST_DIR}/pvreconstruction.cpp)
    target_link_libraries(pvreconstruction PRIVATE ${NANO_LIB} mengu_compiler_flags)
    target_compile_options(pvreconstruction PRIVATE -O0)

    add_executable(lpctest ${ALL_SRC} ${TEST_DIR}/lpctest.cpp)
    target_link_libraries(lpctest PRIVATE ${NANO_LIB} mengu_compiler_flags)

    add_executable(filtertest ${ALL_SRC} ${TEST_DIR}/filtertest.cpp)
    target_link_libraries(filtertest PRIVATE ${NANO_LIB} mengu_compiler_flags)

    # add_executable(mengubahuitest ${ALL_SRC} 
    #     ${TEST_DIR}/mengubahuitest.cpp 
    #     "${MenguPitchy_SOURCE_DIR}/mengubahui.cpp"
    #     "${MenguPitchy_SOURCE_DIR}/mengubahengine.cpp"
    # )
    # target_link_libraries(mengubahuitest PRIVATE ${NANO_LIB} mengu_compiler_flags)
endif()