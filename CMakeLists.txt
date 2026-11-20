cmake_minimum_required(VERSION 3.22)

project(BrouCrusher VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ON = copia el .vst3 a la carpeta VST3 del sistema despues de compilar
option(COPY_AFTER_BUILD "Copiar el plugin a la carpeta VST3 al terminar" ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(BrouCrusher
    COMPANY_NAME "Brou"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Brou
    PLUGIN_CODE Bcru
    FORMATS VST3 Standalone
    PRODUCT_NAME "Brou Crusher"
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(BrouCrusher PRIVATE
    CrusherProcessor.cpp
    CrusherProcessor.h
    CrusherEditor.cpp
    CrusherEditor.h)

target_compile_definitions(BrouCrusher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(BrouCrusher
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
