/*
    GuiTest.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include <GuiSupport.h>
#include <imgui.h>
#include <AppInterface.h>
#include <memory>

class Window : public GuiSupport::Window {
    void setupRendering() override {}

    void renderFrame() override {
        ImGui::Begin("GuiTest");
        if (ImGui::Button("Click me")) {
            GuiSupport::log("Button was pressed");
        }
        ImGui::End();
    }

    void teardownRendering() override {}
};

static std::unique_ptr<Window> window;

void AppInterface::setup() {
    GuiSupport::setSystemLogPrefix("GuiTest");
    window = std::make_unique<Window>();
    window->configureWindowBeforeCreation(640, 480, false, "GuiTest");
}

void AppInterface::teardown() {
    window = nullptr;
}

#if __ANDROID__

void AppInterface::renderFrame(unsigned currentWidth, unsigned currentHeight, float scaleFactor) {
    window->renderFrameInGLSurfaceViewContext(currentWidth, currentHeight, scaleFactor);
}

void AppInterface::updateMouseState(bool down, int x, int y) {
    window->updateMouseState(down, x, y);
}

void AppInterface::plugStructureCoreFileDescriptor(int fd) {}

#else

void AppInterface::runUntilWindowClosed() {
    window->runUntilWindowClosed();
}
#endif

void AppInterface::cliCmdHandler(int argc, char **argv){
    GuiSupport::log("Cli hasn't been supported on this App yet");
}
