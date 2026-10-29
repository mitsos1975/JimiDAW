cmake_minimum_required(VERSION 3.22)
project(JimiDAW VERSION 0.3.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(JIMIDAW_ASIO "Build with Steinberg ASIO. Requires third_party/asiosdk." OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(JimiDAW
    PRODUCT_NAME "JimiDAW"
    COMPANY_NAME "JimiDAW"
    BUNDLE_ID "com.jimidaw.app"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Resources/JimiDAW.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Resources/JimiDAW.png"
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
)

juce_generate_juce_header(JimiDAW)

target_sources(JimiDAW PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/LookAndFeel.h
    Source/LookAndFeel.cpp
    Source/TransportBar.h
    Source/TransportBar.cpp
    Source/TrackList.h
    Source/TrackList.cpp
    Source/ArrangementView.h
    Source/ArrangementView.cpp
    Source/MixerView.h
    Source/MixerView.cpp
    Source/AudioEngine.h
    Source/AudioEngine.cpp
    Source/Project.h
    Source/Session.h
    Source/Session.cpp
    Source/PianoRoll.h
    Source/PianoRoll.cpp
    Source/Inspector.h
    Source/Inspector.cpp
    Source/StudioChrome.h
    Source/StudioChrome.cpp
)

target_compile_definitions(JimiDAW PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:JimiDAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:JimiDAW,JUCE_VERSION>"
)

if(JIMIDAW_ASIO)
    target_compile_definitions(JimiDAW PRIVATE JUCE_ASIO=1)
    target_include_directories(JimiDAW PRIVATE ${CMAKE_SOURCE_DIR}/third_party/asiosdk/common)
else()
    target_compile_definitions(JimiDAW PRIVATE JUCE_ASIO=0)
endif()

target_link_libraries(JimiDAW PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_formats
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
