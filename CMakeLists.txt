cmake_minimum_required(VERSION 3.15)

set(USE_JUCE_DIR "" CACHE PATH "If used, the path to a JUCE checkout directory. If not set, JUCE will be found with find_package.")

project(BrightnessSliderApplet
	LANGUAGES CXX
	VERSION 0.1)
	
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /wd5033")
    add_compile_definitions(NOMINMAX)
endif()

# find JUCE.
# See https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md
# on how this works.
if(USE_JUCE_DIR)
	# use a juce checkout directory directly, with a private build tree
	add_subdirectory(${USE_JUCE_DIR} "${CMAKE_BINARY_DIR}/JUCE")
else()
	# in this case you need a JUCE install on CMAKE_PREFIX_PATH.
	find_package(JUCE CONFIG REQUIRED)
endif()

juce_add_binary_data(brightness_slider_assets
	HEADER_NAME binaries.h
	SOURCES
		assets/brightness16.png
		assets/brightness32.png
		assets/brightness-silhouette.png
		assets/reset.png)

juce_add_gui_app(brightness_slider
	PRODUCT_NAME "Monitor brightness slider")
set_target_properties(brightness_slider
	PROPERTIES OUTPUT_NAME "Brightness control")

target_compile_definitions(brightness_slider
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:brightness_slider,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:brightness_slider,JUCE_VERSION>")

target_sources(brightness_slider
	PRIVATE
		src/main.cpp
		src/brightness.cpp
		${binary_cpp})

target_link_libraries(brightness_slider
	PRIVATE
		brightness_slider_assets
		juce::juce_gui_basics
		juce::juce_gui_extra
	PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (WIN32)
    target_sources(brightness_slider PRIVATE brightness_slider.rc brightness_slider.manifest)
endif()

target_link_libraries(brightness_slider
	PRIVATE
		Dxva2.lib)

install(TARGETS brightness_slider
    RUNTIME DESTINATION . 
    LIBRARY DESTINATION .)
install(FILES $<TARGET_PDB_FILE:brightness_slider> DESTINATION . OPTIONAL)
