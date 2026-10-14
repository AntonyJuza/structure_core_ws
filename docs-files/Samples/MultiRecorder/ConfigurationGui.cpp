/*
    ConfigurationGui.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "MultiRecorder.h"

#include <imgui.h>

static constexpr float kSliderWidth = 200.f;
static constexpr ImGuiWindowFlags kWindowFlags =
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
static constexpr float SLIDER_WIDTH = 200.f;

template<typename T>
bool enumRadioButton(const char* label, T* value, T ownValue)
{
    int intValue = (int)*value;
    bool status = ImGui::RadioButton(label, &intValue, (int)ownValue);
    *value = (T)intValue;
    return status;
}

#define ENUM_RADIO_BUTTON(...) \
    do { if (enumRadioButton(__VA_ARGS__)) { redrawNext(); } } while (false)

void Window::renderConfigurationScreen()
{
    ImGui::Begin("CaptureSession configuration", nullptr, kWindowFlags);
    GuiSupport::layoutCurrentWindowAsFullCanvas();
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 8.f + std::round(29.f / 2.f - ImGui::GetTextLineHeight() / 2.f)));
    if (ImGui::Button("Start Streaming"))
    {
        prepareForStreaming();
        redrawNext();
    }
    ImGui::PopStyleVar();
    ImGui::NewLine();

    ImGui::Checkbox("Stream depth", &_settings.structureCore.depthEnabled);
    ImGui::NewLine();
    ImGui::Indent();
    ImGui::PushItemWidth(kSliderWidth);
    ImGui::SliderFloat("Framerate" "##depthfps", &_settings.structureCore.depthFramerate, 1.f, 30.f, "%.3f FPS");
    ImGui::PopItemWidth();
    ImGui::NewLine();
    ENUM_RADIO_BUTTON("QVGA (320x240)" "##depthres", &_settings.structureCore.depthResolution, ST::StructureCoreDepthResolution::QVGA);
    ENUM_RADIO_BUTTON("VGA (640x480)" "##depthres", &_settings.structureCore.depthResolution, ST::StructureCoreDepthResolution::VGA);
    ENUM_RADIO_BUTTON("SXGA (1280x960)" "##depthres", &_settings.structureCore.depthResolution, ST::StructureCoreDepthResolution::SXGA);
    ImGui::NewLine();
    ImGui::Text("Depth range mode:");
    ENUM_RADIO_BUTTON("Very short" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::VeryShort);
    ENUM_RADIO_BUTTON("Short" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::Short);
    ENUM_RADIO_BUTTON("Medium" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::Medium);
    ENUM_RADIO_BUTTON("Long" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::Long);
    ENUM_RADIO_BUTTON("Very long" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::VeryLong);
    ENUM_RADIO_BUTTON("Hybrid" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::Hybrid);
    ENUM_RADIO_BUTTON("Default" "##depthrange", &_settings.structureCore.depthRangeMode, ST::StructureCoreDepthRangeMode::Default);
    ImGui::NewLine();
    ImGui::Checkbox("Depth correction", &_settings.applyExpensiveCorrection);
    ImGui::NewLine();
    ImGui::Text("Dynamic calibration mode:");
    ENUM_RADIO_BUTTON("Off" "##dynamiccal", &_settings.structureCore.dynamicCalibrationMode, ST::StructureCoreDynamicCalibrationMode::Off);
    ENUM_RADIO_BUTTON("One-shot" "##dynamiccal", &_settings.structureCore.dynamicCalibrationMode, ST::StructureCoreDynamicCalibrationMode::OneShotPersistent);
    ENUM_RADIO_BUTTON("Continuous" "##dynamiccal", &_settings.structureCore.dynamicCalibrationMode, ST::StructureCoreDynamicCalibrationMode::ContinuousNonPersistent);
    ImGui::Unindent();
    ImGui::NewLine();

    ImGui::Checkbox("Stream visible", &_settings.structureCore.visibleEnabled);
    ImGui::NewLine();
    ImGui::Indent();
    ImGui::PushItemWidth(kSliderWidth);
    ImGui::SliderFloat("Framerate" "##visiblefps", &_settings.structureCore.visibleFramerate, 1.f, 30.f, "%.3f FPS");
    ImGui::SliderFloat("Exposure" "##visibleexp", &_settings.structureCore.initialVisibleExposure, 0.001f, 0.033f, "%.3f sec");
    ImGui::SliderFloat("Gain" "##visiblegain", &_settings.structureCore.initialVisibleGain, 1.f, 8.f);
    ImGui::PopItemWidth();
    ImGui::Unindent();
    ImGui::NewLine();

    ImGui::Checkbox("Stream infrared", &_settings.structureCore.infraredEnabled);
    ImGui::NewLine();
    ImGui::Indent();
    ImGui::PushItemWidth(SLIDER_WIDTH);
    ImGui::SliderFloat("Framerate" "##infraredfps", &_settings.structureCore.infraredFramerate, 1.f, 30.f, "%.3f FPS");
    ImGui::SliderFloat("Exposure" "##infraredexp", &_settings.structureCore.initialInfraredExposure, 0.001f, 0.033f, "%.3f sec");
    ImGui::SliderInt("Gain" "##infraredgain", &_settings.structureCore.initialInfraredGain, 0, 3);
    ImGui::PopItemWidth();
    ImGui::Checkbox("Autoexposure" "##infraredae", &_settings.structureCore.infraredAutoExposureEnabled);
    ImGui::Checkbox("Disable intensity balance" "##infrarednoib", &_settings.structureCore.disableInfraredIntensityBalance);
    ImGui::NewLine();
    ImGui::Text("Active cameras:");
    ENUM_RADIO_BUTTON("Left" "##infraredsrc", &_settings.structureCore.infraredMode, ST::StructureCoreInfraredMode::LeftCameraOnly);
    ENUM_RADIO_BUTTON("Right" "##infraredsrc", &_settings.structureCore.infraredMode, ST::StructureCoreInfraredMode::RightCameraOnly);
    ENUM_RADIO_BUTTON("Left + right" "##infraredsrc", &_settings.structureCore.infraredMode, ST::StructureCoreInfraredMode::BothCameras);
    ImGui::Unindent();
    ImGui::NewLine();

    ImGui::Checkbox("Frame sync", &_settings.frameSyncEnabled);
    ImGui::NewLine();
    
    ImGui::End();
}
