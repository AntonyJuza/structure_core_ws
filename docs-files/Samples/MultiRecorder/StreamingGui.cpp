/*
    StreamingGui.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "MultiRecorder.h"

#include <GuiSupport.h>

#include <imgui.h>

#include <sstream>

static const float kSliderWidth = 150.f;
static const ImVec4 red(1.f, 0.f, 0.f, 1.f);
static const ImVec4 yellow(1.f, 1.f, 0.f, 1.f);
static const ImVec4 green(0.f, 1.f, 0.f, 1.f);
static constexpr ImGuiWindowFlags kWindowFlags =
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

static ImVec4 colorForCaptureSessionStatus(ST::CaptureSessionEventId event)
{
    switch (event)
    {
        case ST::CaptureSessionEventId::Booting:
            return yellow;
        case ST::CaptureSessionEventId::Connected:
        case ST::CaptureSessionEventId::Ready:
        case ST::CaptureSessionEventId::Streaming:
            return green;
        default:
            return red;
    }
}

static ImVec4 colorForUSBVersion(ST::CaptureSessionUSBVersion version)
{
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

void Window::renderStreamingScreen()
{
    GuiSupport::GridConfig gridConfig;
    gridConfig.numCellsX = 3;
    gridConfig.numCellsY = _delegate.getAllDeviceData().size();
    gridConfig.numTools = 1;
    gridConfig.toolAreaWidth = 200;

    ImGui::Begin("##Streaming", nullptr, kWindowFlags);
    GuiSupport::layoutCurrentWindowAsGridTool(gridConfig, 0);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f + std::round(29 / 2.f - ImGui::GetTextLineHeight() / 2.f)));
    if (ImGui::Button("Stop Streaming", ImVec2(kSliderWidth, 40)))
    {
        exitStreaming();
    }
    ImGui::PopStyleVar(2);

    ImGui::NewLine();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.f, 2.f + std::round(29 / 2.f - ImGui::GetTextLineHeight() / 2.f)));
    if (_recordingOcc)
    {
        if (ImGui::Button("Stop Recording", ImVec2(kSliderWidth, 40)))
        {
            for (PerDeviceData* data : _delegate.getAllDeviceData())
            {
                if (data->occWriter.isWriting())
                {
                    data->occWriter.finalizeWriting();
                }
            }
            _recordingOcc = false;
        }
    }
    else
    {
        if (ImGui::Button("Record OCC Files", ImVec2(kSliderWidth, 40)))
        {
            static std::string timeStamp;
            if(!_recordingOcc)
            	timeStamp = ST::formattedStringFromLocaltime();
            _recordingOcc = true;

            //FIXME before release: Changed how we save the occ although I think this way is more clear for users.
            std::string folderPath;
            folderPath += "[AppDocuments]/occ/";
            folderPath += timeStamp;
            auto path = ST::resolveSmartPath(folderPath.c_str());
            if (ST::createDirectories(path))
            {
                for (PerDeviceData* data : _delegate.getAllDeviceData())
                {
                    std::stringstream outputOCCFilePathAndName;
                    outputOCCFilePathAndName << folderPath << "/" <<data->parent->sensorInfo().serialNumber << ".occ";
                    data->occWriter.startWritingToFile(ST::resolveSmartPath(outputOCCFilePathAndName.str()).c_str(), *(data->parent));
                }
            }
            else
            {
                GuiSupport::log("Unable to create output directory: %s", path.c_str());
                GuiSupport::log("Will not record OCC.");
                _recordingOcc = false;
            }
        }
    }
    ImGui::PopStyleVar(2);
    
    ImGui::NewLine();
    
    bool visibleParamsChanged = false;
    ImGui::PushItemWidth(kSliderWidth);
    ImGui::Text("Visible exposure:");
    ImGui::SliderFloat("##visibleexp", &_settings.structureCore.initialVisibleExposure, 0.001f, 0.033f, "%.3f sec");
    visibleParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::Text("Visible gain:");
    ImGui::SliderFloat("##visiblegain", &_settings.structureCore.initialVisibleGain, 1.f, 8.f);
    visibleParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopItemWidth();

    ImGui::NewLine();

    bool infraredParamsChanged = false;
    ImGui::PushItemWidth(kSliderWidth);
    ImGui::Text("Infrared exposure:");
    ImGui::SliderFloat("##infraredexp", &_settings.structureCore.initialInfraredExposure, 0.001f, 0.033f, "%.3f sec");
    infraredParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::Text("Infrared gain:");
    ImGui::SliderInt("##infraredgain", &_settings.structureCore.initialInfraredGain, 0, 3);
    infraredParamsChanged |= ImGui::IsItemDeactivatedAfterEdit();
    ImGui::PopItemWidth();

    for (PerDeviceData* data : _delegate.getAllDeviceData())
    {
        ImGui::Text("Capture session status:");
        ST::CaptureSessionEventId sessionStatus = data->lastCaptureSessionEvent;
        ImGui::TextColored(colorForCaptureSessionStatus(sessionStatus), "%s", ST::CaptureSessionSample::toString(sessionStatus));

        ImGui::NewLine();

        ImGui::Text("USB status:");
        ST::CaptureSessionUSBVersion usbVersion = data->parent->USBVersion();
        ImGui::TextColored(colorForUSBVersion(usbVersion), "%s", stringForUSBVersion(usbVersion));

        ImGui::NewLine();

        const double depthRate = data->depthRateMonitor.rate();
        const double infraredRate = data->infraredRateMonitor.rate();
        ImGui::Text("Depth: %.3f", depthRate);
        ImGui::Text("Infrared: %.3f", infraredRate);
        ImGui::NewLine();
    }

    ImGui::End();
    int currentCameraIndex = 0;
    for (PerDeviceData* data : _delegate.getAllDeviceData())
    {
        ST::DepthFrame depthFrame = data->lastDepthFrame;
        if (depthFrame.isValid() && _lastDepthTimestamp != depthFrame.timestamp())
        {
            _lastDepthTimestamp = depthFrame.timestamp();
            data->frameRenderer->renderDepthFrame(depthFrame);
        }

        ST::InfraredFrame infraredFrame = data->lastInfraredFrame;
        if (infraredFrame.isValid() && _lastRenderedInfraredTimestamp != infraredFrame.timestamp()) {
            _lastRenderedInfraredTimestamp = infraredFrame.timestamp();
            data->frameRenderer->renderInfraredFrame(infraredFrame);
        }

        ST::ColorFrame visibleFrame = data->lastVisibleOrColorFrame;
        if (visibleFrame.isValid() && _lastVisibleOrColorTimestamp != visibleFrame.timestamp())
        {
            _lastVisibleOrColorTimestamp = visibleFrame.timestamp();
            data->frameRenderer->renderVisibleFrame(visibleFrame);
        }
 
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

        int visualWindowsIndex = 0;
        if (depthFrame.isValid())
        {
            std::stringstream windowName;
            windowName << "Depth " << currentCameraIndex;

            ImGui::Begin(windowName.str().c_str(), nullptr, kWindowFlags);
            GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, visualWindowsIndex++ , currentCameraIndex);
            GuiSupport::drawTextureInContentArea(data->frameRenderer->depthTexture());
            ImGui::End();
        }
        if (visibleFrame.isValid())
        {
            std::stringstream windowName;
            windowName << "Visible " << currentCameraIndex;

            ImGui::Begin(windowName.str().c_str(), nullptr, kWindowFlags);
            GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, visualWindowsIndex++ , currentCameraIndex);
            GuiSupport::drawTextureInContentArea(data->frameRenderer->visibleTexture());
            ImGui::End();
        }
        if (infraredFrame.isValid())
        {
            std::stringstream windowName;
            windowName << "Infrared" << currentCameraIndex;

            ImGui::Begin(windowName.str().c_str(), nullptr, kWindowFlags);
            GuiSupport::layoutCurrentWindowAsGridCell(gridConfig, visualWindowsIndex++ , currentCameraIndex);
            GuiSupport::drawTextureInContentArea(data->frameRenderer->infraredTexture());
            ImGui::End();
        }
        ImGui::PopStyleVar();
        
        currentCameraIndex++;

        if (visibleParamsChanged)
        {
            data->parent->setVisibleCameraExposureAndGain(
                _settings.structureCore.initialVisibleExposure, _settings.structureCore.initialVisibleGain
            );
        }
        if (infraredParamsChanged)
        {
            data->parent->setInfraredCamerasExposureAndGain(
                _settings.structureCore.initialInfraredExposure, (float)_settings.structureCore.initialInfraredGain
            );
        }
    }
}
