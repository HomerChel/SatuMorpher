cmake_minimum_required(VERSION 3.20)
project(SatuMorpher VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(SatuMorpher
    COMPANY_NAME "SintezaFX"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Lcl1
    PLUGIN_CODE Sat1
    FORMATS VST3
    PRODUCT_NAME "SatuMorpher"
)

juce_add_binary_data(SatuMorpherAssets SOURCES
    assets/wood.png
    assets/logo.png
)

juce_generate_juce_header(SatuMorpher)

target_sources(SatuMorpher PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h
    src/PluginEntry.cpp
)

target_compile_definitions(SatuMorpher PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SatuMorpher PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    SatuMorpherAssets
)
