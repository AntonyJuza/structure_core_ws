/*
    StreamingGui.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "DepthTester.h"

#include <GuiSupport.h>

#include <imgui.h>
#include <errno.h>
#include <string.h>

using DepthTester::Window;

static constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
static constexpr float SLIDER_WIDTH = 150.f;

extern "C"
{
    // StopIcon.c
    extern const unsigned char *const StopIcon_rgba8;
    extern const size_t StopIcon_width;
    extern const size_t StopIcon_height;
};

static ImVec4 colorForCaptureSessionStatus(ST::CaptureSessionEventId event)
{
    static const ImVec4 red(1.f, 0.f, 0.f, 1.f);
    static const ImVec4 yellow(1.f, 1.f, 0.f, 1.f);
    static const ImVec4 green(0.f, 1.f, 0.f, 1.f);
    switch (event)
    {
        case ST::CaptureSessionEventId::Booting:
            return yellow;
        case ST::CaptureSessionEventId::Ready:
        case ST::CaptureSessionEventId::Streaming:
            return green;
        default:
            return red;
    }
}

static ImVec4 colorForUSBVersion(ST::CaptureSessionUSBVersion version)
{
    static const ImVec4 red(1.f, 0.f, 0.f, 1.f);
    static const ImVec4 yellow(1.f, 1.f, 0.f, 1.f);
    static const ImVec4 green(0.f, 1.f, 0.f, 1.f);
    switch (version)
    {
        case ST::CaptureSessionUSBVersion::USB3:
            return green;
        case ST::CaptureSessionUSBVersion::USB2:
            return yellow;
        default:
            return red;
    }
}

const char* stringForUSBVersion(ST::CaptureSessionUSBVersion version)
{
    switch (version)
    {
        case ST::CaptureSessionUSBVersion::USB3:
            return "USB3";
        case ST::CaptureSessionUSBVersion::USB2:
            return "USB2";
        case ST::CaptureSessionUSBVersion::USB1:
            return "USB1";
        default:
            return "ERROR";
    }
}

static void saveFrameToImage(ST::DepthFrame depthFrame, ST::DepthFrame generatedDepthFrame, ST::InfraredFrame infraredFrame)
{
	std::string placedFolderPath = "[AppDocuments]/occ/" + ST::formattedStringFromLocaltime(), \
			depthPNGName = "depthFrame.png", generatedDepthPNGName = "generatedDepthFrame.png", \
			infraredPNGName = "infraredFrame.png", depthPLYName = "depthPointCould.ply", \
			generatedDepthPLYName = "generatedDepthPointCould.ply";

    if (!ST::createDirectories(ST::resolveSmartPath(placedFolderPath.c_str())))
    {
        GuiSupport::log("Unable to create output directory: %s", ST::resolveSmartPath("[AppDocuments]/occ").c_str());
        GuiSupport::log("Will not store images.");
        return;
    }

    std::string depthPNGPath    = ST::resolveSmartPath(placedFolderPath + "/" + depthPNGName);
    std::string generatedDepthPNGPath  = ST::resolveSmartPath(placedFolderPath + "/" + generatedDepthPNGName);
    std::string infraredPNGPath = ST::resolveSmartPath(placedFolderPath + "/" + infraredPNGName);
    std::string depthPLYPath    = ST::resolveSmartPath(placedFolderPath + "/" + depthPLYName);
    std::string generatedDepthPLYPath = ST::resolveSmartPath(placedFolderPath + "/" + generatedDepthPLYName);

    if (depthFrame.isValid()){
        depthFrame.saveImageToPngFile(depthPNGPath.c_str());
        depthFrame.saveImageAsPointCloudMesh(depthPLYPath.c_str());
    }
    if (depthFrame.isValid() && infraredFrame.isValid()){
        generatedDepthFrame.saveImageToPngFile(generatedDepthPNGPath.c_str());
        generatedDepthFrame.saveImageAsPointCloudMesh(generatedDepthPLYName.c_str());
    }
    if (infraredFrame.isValid())
        infraredFrame.saveImageToPngFile(infraredPNGPath.c_str());
    
}

void Window::renderStreamingScreen()
{
    if (_stopIconTextureId == 0)
    {
        _stopIconTextureId = GuiSupport::generateTextureFromRGBA8Data(StopIcon_rgba8, StopIcon_width, StopIcon_height);
    }

    ST::DepthFrame sensorDepthFrame    = _sessionDelegate.lastSensorDepthFrame();
    ST::DepthFrame generatedDepthFrame = _sessionDelegate.lastGeneratedDepthFrame();
    ST::InfraredFrame infraredFrame    = _sessionDelegate.lastInfraredFrame();
    if (sensorDepthFrame.isValid()    &&
        // generatedDepthFrame.isValid() &&
        infraredFrame.isValid()       &&
        _lastRenderedSensorDepthTimestamp != sensorDepthFrame.timestamp())
    {
        _lastRenderedSensorDepthTimestamp = sensorDepthFrame.timestamp();
        _frameRenderer->renderDepthFrame(sensorDepthFrame);
        _frameRenderer->renderGenDepthFrame(generatedDepthFrame);
        _frameRenderer->renderInfraredFrame(infraredFrame);
    }

    double depthRate = _sessionDelegate.depthRate();
    double infraredRate = _sessionDelegate.infraredRate();

    GuiSupport::GridConfig gridConfig;
    gridConfig.numCellsX = 2;
    gridConfig.numCellsY = 2;
    gridConfig.numTools = 1;
    gridConfig.toolAreaWidth = 200;

    ImGui::Begin("##Streaming", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsGridTool(gridConfig, 0);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    if (ImGui::ImageButton((void *)(uintptr_t)_stopIconTextureId, ImVec2(StopIcon_width, StopIcon_height), ImVec2(0, 0), ImVec2(1, 1), 8))
    {
        exitStreaming();
    }
    ImGui::PopStyleVar();

    ImGui::NewLine();

    ImGui::Text("Capture session status:");
    ST::CaptureSessionEventId sessionStatus = _sessionDelegate.lastCaptureSessionEvent();
    ImGui::TextColored(colorForCaptureSessionStatus(sessionStatus), "%s", ST::CaptureSessionSample::toString(sessionStatus));

    ImGui::NewLine();

    ImGui::Text("USB status:");
    ST::CaptureSessionUSBVersion usbVersion = _captureSession.USBVersion();
    ImGui::TextColored(colorForUSBVersion(usbVersion), "%s", stringForUSBVersion(usbVersion));

    ImGui::NewLine();

    bool infraredParamsChanged = false;
    ImGui::PushItemWidth(SLIDER_WIDTH);
    ImGui::Text("Infrared exposure:");
    ImGui::SliderFloat("##infraredexp", &_settings.structureCore.initialInfraredExposure, 0.001f, 0.033f, "%.3f sec");
    infraredParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::Text("Infrared gain:");
    ImGui::SliderInt("##infraredgain", &_settings.structureCore.initialInfraredGain, 0, 3);
    infraredParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopItemWidth();

    ImGui::NewLine();

    ImGui::Text("Sample rates (Hz):");
    ImGui::Text("   Depth: %.3f", depthRate);
    ImGui::Text("Infrared: %.3f", infraredRate);

    ImGui::NewLine();
    auto info = _captureSession.sensorInfo();
    ImGui::Text("NU3000 Die Temp: %.3f", info.structureCore.temperature);

    ImGui::NewLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f + std::round(29 / 2.f - ImGui::GetTextLineHeight() / 2.f)));
    if (_sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::NU3000_DEPTH).isWriting() || _sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::HOST_PROCESSED_DEPTH).isWriting())
    {
        if (ImGui::Button("Stop Recording", ImVec2(SLIDER_WIDTH, 40)))
        {
            _sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::NU3000_DEPTH).finalizeWriting();
            _sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::HOST_PROCESSED_DEPTH).finalizeWriting();
        }
    }
    else
    {
        ImGui::NewLine();
        // ImGui::Indent();
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f + std::round(29 / 2.f - ImGui::GetTextLineHeight() / 2.f)));
        if (ImGui::Button("Save Images & PLY", ImVec2(SLIDER_WIDTH, 40))) {
            saveFrameToImage(sensorDepthFrame, generatedDepthFrame, infraredFrame);
        }
        ImGui::PopStyleVar(2);
        
        ImGui::NewLine();
        if (ImGui::Button("Record OCC File", ImVec2(SLIDER_WIDTH, 40)))
        {
            auto path = ST::resolveSmartPath("[AppDocuments]/occ");
            if (ST::createDirectories(path))
            {
                std::string timeStr = ST::formattedStringFromLocaltime();
        	    std::string outputOCCFilePathAndName = "[AppDocuments]/occ/StructureCore_" + timeStr + ".occ";
                _sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::NU3000_DEPTH).startWritingToFile(ST::resolveSmartPath(outputOCCFilePathAndName).c_str(), _captureSession);
                
                outputOCCFilePathAndName = "[AppDocuments]/occ/GeneratedDepth_" + timeStr + ".occ";
                _sessionDelegate.occFileWriter(DepthTester::DEPTH_CAT::HOST_PROCESSED_DEPTH).startWritingToFile(ST::resolveSmartPath(outputOCCFilePathAndName).c_str(), _captureSession);
            }
            else
            {
                GuiSupport::log("Unable to create output directory: %s", path.c_str());
                GuiSupport::log("Will not record OCC.");
            }
        }
    }
    ImGui::PopStyleVar(2);

    ImGui::End();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGui::Begin("Infrared", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, 0, 0, 2, 1);
    GuiSupport::drawTextureInContentArea(_frameRenderer->infraredTexture());
    ImGui::End();

    ImGui::Begin("Sensor Depth", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, 0, 1);
    GuiSupport::drawTextureInContentArea(_frameRenderer->depthTexture());
    ImGui::End();

    ImGui::Begin("Generated Depth", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, 1, 1);
    GuiSupport::drawTextureInContentArea(_frameRenderer->genDepthTexture());
    ImGui::End();

    ImGui::PopStyleVar();

    if (infraredParamsChanged)
    {
        _captureSession.setInfraredCamerasExposureAndGain(
            _settings.structureCore.initialInfraredExposure,
            (float)_settings.structureCore.initialInfraredGain
        );
    }
}
