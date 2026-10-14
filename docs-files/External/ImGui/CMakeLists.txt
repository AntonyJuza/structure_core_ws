include(${SCSDK_ROOT}/CMake/project.cmake)

add_library(ImGui STATIC
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_widgets.cpp
)
if(SCSDK_IS_ANDROID)
    target_link_libraries(ImGui PRIVATE
        GLESv3
    )
    target_sources(ImGui PRIVATE
        src/examples/imgui_impl_opengl3.cpp
    )
    target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_CUSTOM=1)
else()
    target_link_libraries(ImGui PRIVATE
        scsdk_glew
        scsdk_glfw
    )
    target_sources(ImGui PRIVATE
        src/examples/imgui_impl_glfw.cpp
        src/examples/imgui_impl_opengl3.cpp
    )
    target_compile_definitions(ImGui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLEW=1)
endif()
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
