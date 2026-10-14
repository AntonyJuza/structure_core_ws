/*
    Window_Android.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Window.h"
#include "Utility.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <imgui.h>
#include <examples/imgui_impl_opengl3.h>
#include <GLES3/gl3.h>
using Window = GuiSupport::Window;

static constexpr size_t MOUSE_QUEUE_MAX = 64;
namespace {
    struct MouseEvent {
        bool down = false;
        int x = 0;
        int y = 0;
    };
}

struct Window::Impl {
    bool didInitRendering = false;
    unsigned currentWidth = 0;
    unsigned currentHeight = 0;

    std::mutex mouseLock;
    std::queue<MouseEvent> mouseQueue;
};

static thread_local Window *_localInstance = nullptr;

static void checkCurrent(Window *self) {
    if (_localInstance != self) {
        throw std::runtime_error("This window is not current! Must be called within renderFrame().");
    }
}

Window::Window() {
    _impl = std::make_unique<Impl>();
}

Window::~Window() {}

void Window::configureWindowBeforeCreation(unsigned initialWidth, unsigned initialHeight, bool maximize, const std::string& title) {
    // Ignored
}

void Window::wake() {
    // No-op; GLSurfaceView renders continuously
}

Window& Window::current() {
    if (_localInstance) {
        return *_localInstance;
    }
    else {
        throw std::runtime_error("No window is current! Must be called within renderFrame().");
    }
}

void Window::redrawNext() {
    checkCurrent(this);
    // No-op; GLSurfaceView renders continuously
}

unsigned Window::windowWidth() {
    checkCurrent(this);
    return _impl->currentWidth;
}

unsigned Window::windowHeight() {
    checkCurrent(this);
    return _impl->currentHeight;
}

void Window::renderFrameInGLSurfaceViewContext(unsigned currentWidth, unsigned currentHeight, float scaleFactor) {
    if (_localInstance) {
        throw std::runtime_error("Tried to render a frame while already rendering a frame!");
    }
    _localInstance = this;

    if (!_impl->didInitRendering) {
        _impl->didInitRendering = true;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplOpenGL3_Init("#version 300 es");

        // Suppress persistent state unless implementation wants it
        ImGui::GetIO().IniFilename = nullptr;

        setupRendering();
    }
    _impl->currentWidth = (unsigned)(currentWidth / scaleFactor);
    _impl->currentHeight = (unsigned)(currentHeight / scaleFactor);

    ImGui::GetIO().DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);
    ImGui::GetIO().DisplaySize = ImVec2((float)_impl->currentWidth, (float)_impl->currentHeight);
    {
        std::unique_lock<std::mutex> u(_impl->mouseLock);
        if (!_impl->mouseQueue.empty()) {
            MouseEvent ev = _impl->mouseQueue.front();
            _impl->mouseQueue.pop();
            ImGui::GetIO().MouseDown[0] = ev.down;
            ImGui::GetIO().MousePos = ImVec2((float)ev.x / scaleFactor, (float)ev.y / scaleFactor);
        }
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    renderFrame();
    ImGui::Render();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    _localInstance = nullptr;
}

void Window::updateMouseState(bool down, int x, int y) {
    std::unique_lock<std::mutex> u(_impl->mouseLock);
    if (_impl->mouseQueue.size() < MOUSE_QUEUE_MAX) {
        _impl->mouseQueue.push(MouseEvent {down, x, y});
    }
}
