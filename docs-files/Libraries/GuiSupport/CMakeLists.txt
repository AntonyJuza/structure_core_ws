add_library(GuiSupport STATIC
    GuiSupport.h
    Drawing.h Drawing.cpp
    FileSelector.h FileSelector.cpp
    Layout.h Layout.cpp
    Window.h
    Style.h Style.cpp
    Utility.h Utility.cpp
    CommandLine.h CommandLine.cpp
)
use_scsdk_defaults_for(GuiSupport)
target_link_libraries(GuiSupport
    PUBLIC ImGui
)
target_include_directories(GuiSupport INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(SCSDK_IS_ANDROID)
    target_link_libraries(GuiSupport PRIVATE
        GLESv3
        log
    )
    target_sources(GuiSupport PRIVATE
        Window_Android.cpp
    )
else()
    target_link_libraries(GuiSupport PRIVATE
        scsdk_glew
        scsdk_glfw
    )
    target_sources(GuiSupport PRIVATE
        Window_Desktop.cpp
    )
endif()
if(SCSDK_IS_MACOS)
    target_sources(GuiSupport PRIVATE
        Apple.h Apple.mm
    )
    set_source_files_properties(Apple.mm PROPERTIES
        LANGUAGE CXX
        CXX_FLAGS "-x objective-c++ -fobjc-arc"
    )
endif()
