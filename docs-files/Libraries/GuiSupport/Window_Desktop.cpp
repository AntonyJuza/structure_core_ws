/*
    Window_Desktop.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Window.h"
#include "Utility.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <stdio.h>
#include <imgui.h>
#include <examples/imgui_impl_glfw.h>
#include <examples/imgui_impl_opengl3.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#if __APPLE__
#include "Apple.h"
#endif
using Window = GuiSupport::Window;

struct Window::Impl {
    struct GLFWwindow *window = nullptr;
    unsigned redrawCount = 0;

    unsigned initialWidth = 640;
    unsigned initialHeight = 480;
    bool maximize = false;
    std::string title = "untitled";
};

static thread_local Window *_currentWindow = nullptr;

static void handleGlfwError(int err, const char *desc) {
    GuiSupport::log("GLFW error %d: %s", err, desc);
    exit(1);
}

Window::Window() {
    static std::once_flag once;
    std::call_once(once, []() {
        glfwSetErrorCallback(handleGlfwError);
        glfwInit();
    });
    _impl = std::make_unique<Impl>();
}

Window::~Window() {}

static void checkInRenderImpl(Window *self) {
    if (_currentWindow != self) {
        throw std::runtime_error("Called a window function that may only be called within renderFrame()");
    }
}

void Window::redrawNext() {
    checkInRenderImpl(this);
    _impl->redrawCount = std::max(2u, _impl->redrawCount);
}

void Window::configureWindowBeforeCreation(unsigned initialWidth, unsigned initialHeight, bool maximize, const std::string& title) {
    _impl->initialWidth = initialWidth;
    _impl->initialHeight = initialHeight;
    _impl->maximize = maximize;
    _impl->title = title;
}

void Window::runUntilWindowClosed() {
    if (_currentWindow) {
        throw std::runtime_error("Running a window within another window is unsupported");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, _impl->maximize);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
    _impl->window = glfwCreateWindow((int)_impl->initialWidth, (int)_impl->initialHeight, _impl->title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(_impl->window);
    glfwSwapInterval(1);

    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui_ImplGlfw_InitForOpenGL(_impl->window, true);

    // Suppress persistent state unless implementation wants it
    ImGui::GetIO().IniFilename = nullptr;

    setupRendering();

    _currentWindow = this;
    _impl->redrawCount = 2;
#if __APPLE__
    bool didInitialNSGLUpdate = false;
#endif
    while (!glfwWindowShouldClose(_impl->window)) {
        if (_impl->redrawCount == 0) {
            glfwWaitEventsTimeout(1.0);
        }
        else {
            --_impl->redrawCount;
            glfwPollEvents();
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderFrame();
        ImGui::Render();

        int width = 0, height = 0;
        glfwGetFramebufferSize(_impl->window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#if __APPLE__
        // FIXME: This is a workaround for a known GLFW issue that has yet to be
        // resolved on the stable branch. See https://github.com/glfw/glfw/issues/1334
        if (!didInitialNSGLUpdate) {
            didInitialNSGLUpdate = true;
            GuiSupport::updateNSGLContextForWindow(_impl->window);
        }
#endif
        glfwSwapBuffers(_impl->window);
    }

    teardownRendering();

    glfwDestroyWindow(_impl->window);
    _currentWindow = nullptr;
}

void Window::wake() {
    glfwPostEmptyEvent();
}

Window& Window::current() {
    if (_currentWindow) {
        return *_currentWindow;
    }
    else {
        throw std::runtime_error("There is no current window in this context. Must be called within renderFrame()");
    }
}

unsigned Window::windowWidth() {
    checkInRenderImpl(this);
    int fbWidth = 0, fbHeight = 0;
    glfwGetWindowSize(_impl->window, &fbWidth, &fbHeight);
    return (unsigned)fbWidth;
}

unsigned Window::windowHeight() {
    checkInRenderImpl(this);
    int fbWidth = 0, fbHeight = 0;
    glfwGetWindowSize(_impl->window, &fbWidth, &fbHeight);
    return (unsigned)fbHeight;
}
