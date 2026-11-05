cmake_minimum_required(VERSION 3.25)

# arm64 for daily work; CI overrides this for the universal release build (SELL-01).
# CACHE without FORCE so -D overrides win.
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures")
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Kikkuh VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.5
    GIT_SHALLOW TRUE)
FetchContent_Declare(clap-juce-extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE clap-juce-extensions Catch2)

option(KIKKUH_COPY_PLUGIN "Install the plugin into the user's plug-in folders after building" ON)

# PERMANENT from the first release (DECISIONS 2026-11-03): hosts store these in saved projects.
# Changing any of them afterwards makes every project using Kikkuh report "plugin missing".
juce_add_plugin(Kikkuh
    COMPANY_NAME "Colourmotion"
    BUNDLE_ID com.colourmotion.kikkuh
    PLUGIN_MANUFACTURER_CODE Cmot
    PLUGIN_CODE Kik1
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Drum
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    FORMATS VST3 AU
    PRODUCT_NAME Kikkuh
    COPY_PLUGIN_AFTER_BUILD ${KIKKUH_COPY_PLUGIN})

clap_juce_extensions_plugin(TARGET Kikkuh
    CLAP_ID com.colourmotion.kikkuh
    CLAP_FEATURES instrument drum synthesizer)

# FAST-05: the panel's typeface, Archivo in four instanced cuts (OFL, see assets/fonts/OFL.txt).
juce_add_binary_data(KikkuhAssets SOURCES
    assets/fonts/Archivo-Name.ttf
    assets/fonts/Archivo-Title.ttf
    assets/fonts/Archivo-Label.ttf
    assets/fonts/Archivo-Value.ttf)

# Plugin code shared by the plugin and the tests (no ODR clash: an INTERFACE library).
add_library(KikkuhCode INTERFACE)
file(GLOB_RECURSE KIKKUH_SOURCES CONFIGURE_DEPENDS source/*.cpp source/*.h)
target_sources(KikkuhCode INTERFACE ${KIKKUH_SOURCES})
target_include_directories(KikkuhCode INTERFACE source)
target_compile_definitions(KikkuhCode INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(KikkuhCode INTERFACE
    KikkuhAssets
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

option(KIKKUH_ASAN "Build with AddressSanitizer" OFF)
if(KIKKUH_ASAN)
    target_compile_options(KikkuhCode INTERFACE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(KikkuhCode INTERFACE -fsanitize=address)
endif()

target_link_libraries(Kikkuh PRIVATE KikkuhCode)

# HARN-04: renders a kick to a WAV without a DAW. A development tool, not shipped.
add_executable(kikkuh-render tools/kikkuh-render.cpp)
target_compile_definitions(kikkuh-render PRIVATE $<TARGET_PROPERTY:Kikkuh,COMPILE_DEFINITIONS>)
target_include_directories(kikkuh-render PRIVATE tests)
target_link_libraries(kikkuh-render PRIVATE KikkuhCode)

# HARN-05: prints the numbers of a kick from a WAV or AIFF. A development tool, not shipped.
add_executable(kikkuh-measure tools/kikkuh-measure.cpp)
target_compile_definitions(kikkuh-measure PRIVATE $<TARGET_PROPERTY:Kikkuh,COMPILE_DEFINITIONS>)
target_include_directories(kikkuh-measure PRIVATE tests)
target_link_libraries(kikkuh-measure PRIVATE KikkuhCode)

# HARN-07: turns a round file into blind, level-matched WAVs for listening. A development tool, not shipped.
add_executable(kikkuh-round tools/kikkuh-round.cpp)
target_compile_definitions(kikkuh-round PRIVATE $<TARGET_PROPERTY:Kikkuh,COMPILE_DEFINITIONS>)
target_include_directories(kikkuh-round PRIVATE tests tools)
target_link_libraries(kikkuh-round PRIVATE KikkuhCode)

enable_testing()
file(GLOB_RECURSE KIKKUH_TESTS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(Tests ${KIKKUH_TESTS})
target_compile_definitions(Tests PRIVATE $<TARGET_PROPERTY:Kikkuh,COMPILE_DEFINITIONS>)
target_compile_definitions(Tests PRIVATE "KIKKUH_RENDER_PATH=\"$<TARGET_FILE:kikkuh-render>\"")
target_link_libraries(Tests PRIVATE KikkuhCode Catch2::Catch2WithMain)
target_compile_definitions(Tests PRIVATE "KIKKUH_MEASURE_PATH=\"$<TARGET_FILE:kikkuh-measure>\"")
target_compile_definitions(Tests PRIVATE "KIKKUH_ROUND_PATH=\"$<TARGET_FILE:kikkuh-round>\"")
target_include_directories(Tests PRIVATE tools)
add_dependencies(Tests kikkuh-render kikkuh-measure kikkuh-round)
include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(Tests DISCOVERY_MODE PRE_TEST)
