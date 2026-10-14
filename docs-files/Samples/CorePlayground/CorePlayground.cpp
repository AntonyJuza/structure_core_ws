/*
    CorePlayground.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "CorePlayground.h"
#include <AppInterface.h>
#include <ST/Utilities.h>
#include <imgui.h>
#include <chrono>
#include <thread>
using CorePlayground::Window;

Window::Window() : _sessionDelegate(this) {
    _captureSession.setDelegate(&_sessionDelegate);
    _settings = defaultCaptureSessionSettings();
}

Window::~Window() {}

void Window::setupRendering() {
    ImGui::StyleColorsDark();
    ImGui::GetStyle().WindowPadding = ImVec2(16.f, 16.f);
    ImGui::GetStyle().WindowRounding = 0.f;
    ImGui::GetStyle().WindowBorderSize = 0.f;
    static const ImVec4 borderColor(0.1f, 0.1f, 0.1f, 1.f);
    ImGui::GetStyle().Colors[ImGuiCol_Border] = borderColor;
    ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = borderColor;
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = borderColor;
    GuiSupport::setButtonDisabledColor(ImVec4(0.1f, 0.1f, 0.1f, 1.f));

    // FrameRenderer must be created within the GL context
    _frameRenderer = std::make_unique<SampleProcessing::FrameRenderer>();
}

void Window::renderFrame() {
    Screen currentScreen = _screen.load();
    switch (currentScreen) {
        case Screen::Configuration: renderConfigurationScreen(); break;
        case Screen::LoadConfiguration: renderLoadConfigurationScreen(); break;
        case Screen::SaveConfiguration: renderSaveConfigurationScreen(); break;
        case Screen::ConfigurationError: renderConfigurationErrorScreen(); break;
        case Screen::Streaming: renderStreamingScreen(); break;
        default:;
    }
}

void Window::teardownRendering() {
    // FrameRenderer must be destroyed within the GL context
    _frameRenderer = nullptr;
}

void Window::prepareForStreaming() {
    /* Apply the user's settings to the CaptureSession. The session delegate
       will receive the Ready event once the sensor is ready to stream. */
    _captureSession.startMonitoring(_settings);

    // configure the depth shader to render the depth range appropriate
    // to the current range mode specified
    float minDepthMm = NAN, maxDepthMm = NAN;
    ST::CaptureSessionSettings::minMaxDepthInMmOfDepthRangeMode(
        _settings.structureCore.depthRangeMode, minDepthMm, maxDepthMm
    );
    _frameRenderer->setMinMaxDepthInMm(minDepthMm, maxDepthMm);

    // Switch to the Streaming screen and ensure it renders immediately.
    _screen.store(Screen::Streaming);
    redrawNext();
}

void Window::captureSessionReadyToStream() {
    /* Only start streaming on the right screen. Once the CaptureSession has
       been configured, it will emit a Ready event whenever the sensor is
       connected and comes online. */
    if (_screen.load() == Screen::Streaming) {
        _captureSession.startStreaming();
    }
}

void Window::captureSessionIsStreaming() {
    // if we're running a depth range preset, the exposure/gain values will
    // differ from the default, so we should update those values
    {
        float newExposure = NAN, newGain = NAN;
        _captureSession.getInfraredCamerasExposureAndGain(&newExposure, &newGain);
        if (std::isfinite(newExposure) && std::isfinite(newGain))
        {
            _settings.structureCore.initialInfraredExposure = newExposure;
            _settings.structureCore.initialInfraredGain = newGain;
        }
    }

    wake();
}

void Window::exitStreaming() {
    // Stop the capture session.
    _captureSession.stopStreaming();

    /*To implement the real "one-shot" calibration*/
    if(_settings.structureCore.dynamicCalibrationMode == ST::StructureCoreDynamicCalibrationMode::OneShotPersistent){
    	_settings.structureCore.dynamicCalibrationMode = ST::StructureCoreDynamicCalibrationMode::Off;
    }
    // Switch to the Configuration screen and ensure it renders immediately.
    _screen.store(Screen::Configuration);
    redrawNext();
}

void Window::plugStructureCoreFileDescriptor(int fileDescriptor)
{
    _settings.structureCore.usbFileDescriptor = fileDescriptor;
}

ST::CaptureSession& Window::curCaptureSession(){
    return _captureSession;
}
static std::unique_ptr<Window> window;

void AppInterface::setup() {
    GuiSupport::setSystemLogPrefix("CorePlayground");
    window = std::make_unique<Window>();
    window->configureWindowBeforeCreation(800, 600, false /* maximize */, "CorePlayground");
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

void AppInterface::plugStructureCoreFileDescriptor(int fd) {
    window->plugStructureCoreFileDescriptor(fd);
}

#else

void AppInterface::runUntilWindowClosed() {
    window->runUntilWindowClosed();
}

void AppInterface::cliCmdHandler(int argc, char **argv){
    GuiSupport::log("Cli hasn't been supported on this App yet");
}

#endif
