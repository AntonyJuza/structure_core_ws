/*
    ConfigurationGui.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "DepthTester.h"

#include <ST/Utilities.h>

#include <GuiSupport.h>
#include <imgui.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <fstream>
#include <sstream>

using DepthTester::Window;
using GuiSupport::FileSelector;
using JSON = nlohmann::json;

static constexpr ImGuiWindowFlags WINDOW_FLAGS = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
static constexpr float SLIDER_WIDTH = 200.f;

extern "C"
{
    // PlayIcon.c
    extern const unsigned char *const PlayIcon_rgba8;
    extern const size_t PlayIcon_width;
    extern const size_t PlayIcon_height;
};

namespace
{
    template<typename T> bool EnumRadioButton(const char *label, T *value, T ownValue)
    {
        int iv = (int)*value;
        bool ret = ImGui::RadioButton(label, &iv, (int)ownValue);
        *value = (T)iv;
        return ret;
    }
}

/* When clicked, radio button sets do not redraw until the next frame renders.
   This macro forces a redraw when the value changes. */
#define ENUM_RADIO_BUTTON(...) do {if (EnumRadioButton(__VA_ARGS__)) {redrawNext();}} while (0)

void Window::renderConfigurationScreen()
{
    if (_playIconTextureId == 0)
    {
        _playIconTextureId = GuiSupport::generateTextureFromRGBA8Data(PlayIcon_rgba8, PlayIcon_width, PlayIcon_height);
    }

    ImGui::Begin("Capture session configuration", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsFullCanvas();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.f, 8.f));
    if (ImGui::ImageButton((void *)(uintptr_t)_playIconTextureId, ImVec2(PlayIcon_width, PlayIcon_height), ImVec2(0, 0), ImVec2(1, 1)))
    {
        prepareForStreaming();
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleVar();

    ImGui::NewLine();

    ImGui::Checkbox("Stream depth", &_settings.structureCore.depthEnabled);
    ImGui::NewLine();
    ImGui::Indent();
    ImGui::PushItemWidth(SLIDER_WIDTH);
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

namespace
{
    template<typename T>
    void loadEnumFromJSON(
        const JSON& json, T& field, const std::string& key, const std::string& stringForEnumValue, T enumValue
    )
    {
        if (json.find(key) != json.end() && json[key].is_string() && json[key].get<std::string>() == stringForEnumValue)
        {
            field = enumValue;
        }
    }

    void loadBoolFromJSON(const JSON& json, bool& field, const std::string& key)
    {
        if (json.find(key) != json.end() && json[key].is_boolean())
        {
            field = json[key].get<bool>();
        }
    }

    void loadIntFromJSON(const JSON& json, int& field, const std::string& key)
    {
        if (json.find(key) != json.end() && json[key].is_number())
        {
            field = json[key].get<int>();
        }
    }

    void loadFloatFromJSON(const JSON& json, float& field, const std::string& key)
    {
        if (json.find(key) != json.end() && json[key].is_number())
        {
            field = json[key].get<float>();
        }
    }

    template<typename T>
    void saveEnumToJSON(
        JSON& json, const T& field, const std::string& key, const std::string& stringForEnumValue, T enumValue
    )
    {
        if (field == enumValue)
        {
            json[key] = stringForEnumValue;
        }
    }

    void saveBoolToJSON(JSON& json, const bool& field, const std::string& key)
    {
        json[key] = field;
    }

    void saveIntToJSON(JSON& json, const int& field, const std::string& key)
    {
        json[key] = field;
    }

    void saveFloatToJSON(JSON& json, const float& field, const std::string& key)
    {
        json[key] = field;
    }
}

#define FIELD_DEFINITIONS \
    X_BOOL(structureCore.depthEnabled) \
    X_ENUM(structureCore.depthResolution, QVGA) \
    X_ENUM(structureCore.depthResolution, VGA) \
    X_ENUM(structureCore.depthResolution, SXGA) \
    X_FLOAT(structureCore.depthFramerate) \
    X_ENUM(structureCore.depthRangeMode, VeryShort) \
    X_ENUM(structureCore.depthRangeMode, Short) \
    X_ENUM(structureCore.depthRangeMode, Medium) \
    X_ENUM(structureCore.depthRangeMode, Long) \
    X_ENUM(structureCore.depthRangeMode, VeryLong) \
    X_ENUM(structureCore.depthRangeMode, Hybrid) \
    X_ENUM(structureCore.depthRangeMode, Default) \
    X_BOOL(applyExpensiveCorrection) \
    X_ENUM(structureCore.dynamicCalibrationMode, Off) \
    X_ENUM(structureCore.dynamicCalibrationMode, OneShotPersistent) \
    X_ENUM(structureCore.dynamicCalibrationMode, ContinuousNonPersistent) \
    \
    X_BOOL(structureCore.visibleEnabled) \
    X_FLOAT(structureCore.visibleFramerate) \
    X_FLOAT(structureCore.initialVisibleExposure) \
    X_FLOAT(structureCore.initialVisibleGain) \
    \
    X_BOOL(structureCore.infraredEnabled) \
    X_FLOAT(structureCore.infraredFramerate) \
    X_FLOAT(structureCore.initialInfraredExposure) \
    X_INT(structureCore.initialInfraredGain) \
    X_ENUM(structureCore.infraredMode, LeftCameraOnly) \
    X_ENUM(structureCore.infraredMode, RightCameraOnly) \
    X_ENUM(structureCore.infraredMode, BothCameras) \
    \
    X_BOOL(frameSyncEnabled) \
    \
    X_BOOL(structureCore.accelerometerEnabled) \
    X_BOOL(structureCore.gyroscopeEnabled) \
    X_ENUM(structureCore.imuUpdateRate, AccelAndGyro_100Hz) \
    X_ENUM(structureCore.imuUpdateRate, AccelAndGyro_200Hz) \
    X_ENUM(structureCore.imuUpdateRate, AccelAndGyro_400Hz) \
    X_ENUM(structureCore.imuUpdateRate, AccelAndGyro_1000Hz)

static bool loadConfigurationJSON(
    ST::CaptureSessionSettings& outSettings, const std::string& jsonstr, const ST::CaptureSessionSettings& baseSettings
)
{
    ST::CaptureSessionSettings settings = baseSettings;
    JSON json;
    try
    {
        json = JSON::parse(jsonstr);
    }
    catch (const JSON::parse_error& e)
    {
        GuiSupport::log("Error parsing JSON: %s", e.what());
        return false;
    }

#define X_ENUM(field, enumValue) loadEnumFromJSON(json, settings.field, #field, #enumValue, decltype(settings.field)::enumValue);
#define X_BOOL(field) loadBoolFromJSON(json, settings.field, #field);
#define X_INT(field) loadIntFromJSON(json, settings.field, #field);
#define X_FLOAT(field) loadFloatFromJSON(json, settings.field, #field);
    FIELD_DEFINITIONS
#undef X_ENUM
#undef X_BOOL
#undef X_INT
#undef X_FLOAT

    outSettings = settings;
    return true;
}

static std::string saveConfigurationJSON(const ST::CaptureSessionSettings& settings)
{
    JSON json;
#define X_ENUM(field, enumValue) saveEnumToJSON(json, settings.field, #field, #enumValue, decltype(settings.field)::enumValue);
#define X_BOOL(field) saveBoolToJSON(json, settings.field, #field);
#define X_INT(field) saveIntToJSON(json, settings.field, #field);
#define X_FLOAT(field) saveFloatToJSON(json, settings.field, #field);
    FIELD_DEFINITIONS
#undef X_ENUM
#undef X_BOOL
#undef X_INT
#undef X_FLOAT
    return json.dump(4);
}

void Window::renderLoadConfigurationScreen()
{
    if (!_fileSelector)
    {
        _fileSelector = std::make_unique<FileSelector>(FileSelector::Mode::OpenExistingFile);
    }

    ImGui::Begin("Load configuration file", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsFullCanvas();
    _fileSelector->renderInterior();
    ImGui::End();

    if (_fileSelector->result() == FileSelector::Result::Complete)
    {
        std::ostringstream os;
        FILE *fp = fopen(_fileSelector->resultPath().c_str(), "rb");
        if (!fp)
        {
            _errorMessage = GuiSupport::formatString("Failed to open file: %s: %s", _fileSelector->resultPath().c_str(), strerror(errno));
            goto fail;
        }
        for (char buf[1024]; !feof(fp) && !ferror(fp);)
        {
            size_t n = fread(buf, 1, sizeof buf - 1, fp);
            buf[n] = 0;
            os << buf;
        }
        if (ferror(fp))
        {
            _errorMessage = GuiSupport::formatString("Failed to read file: %s: %s", _fileSelector->resultPath().c_str(), strerror(errno));
            fclose(fp);
            goto fail;
        }
        fclose(fp);
        if (!loadConfigurationJSON(_settings, os.str(), defaultCaptureSessionSettings()))
        {
            _errorMessage = GuiSupport::formatString("Failed to parse JSON in file: %s", _fileSelector->resultPath().c_str());
            goto fail;
        }

        _fileSelector = nullptr;
        _screen.store(Screen::Configuration);
        redrawNext();
        return;

    fail:
        _fileSelector = nullptr;
        _screen.store(Screen::ConfigurationError);
        redrawNext();
    }
    else if (_fileSelector->result() == FileSelector::Result::Cancelled)
    {
        _fileSelector = nullptr;
        _screen.store(Screen::Configuration);
        redrawNext();
    }
}

void Window::renderSaveConfigurationScreen()
{
    if (!_fileSelector)
    {
        _fileSelector = std::make_unique<FileSelector>(FileSelector::Mode::SaveNewFile, ST::resolveSmartPath("[AppDocuments]"), "CaptureSessionSettings.json");
    }

    ImGui::Begin("Save configuration file", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsFullCanvas();
    _fileSelector->renderInterior();
    ImGui::End();

    if (_fileSelector->result() == FileSelector::Result::Complete)
    {
        std::string json = saveConfigurationJSON(_settings);
        FILE *fp = fopen(_fileSelector->resultPath().c_str(), "wb");
        if (!fp)
        {
            _errorMessage = GuiSupport::formatString("Failed to open file: %s: %s", _fileSelector->resultPath().c_str(), strerror(errno));
            goto fail;
        }
        for (size_t wcount = 0; !ferror(fp) && wcount < json.size();)
        {
            size_t n = fwrite(json.data() + wcount, 1, json.size() - wcount, fp);
            wcount += n;
        }
        if (ferror(fp))
        {
            _errorMessage = GuiSupport::formatString("Failed to write file: %s: %s", _fileSelector->resultPath().c_str(), strerror(errno));
            fclose(fp);
            goto fail;
        }
        fclose(fp);

        _fileSelector = nullptr;
        _screen.store(Screen::Configuration);
        redrawNext();
        return;

    fail:
        _fileSelector = nullptr;
        _screen.store(Screen::ConfigurationError);
        redrawNext();
    }
    else if (_fileSelector->result() == FileSelector::Result::Cancelled)
    {
        _fileSelector = nullptr;
        _screen.store(Screen::Configuration);
        redrawNext();
    }
}

void Window::renderConfigurationErrorScreen()
{
    ImGui::Begin("Error", nullptr, WINDOW_FLAGS);
    GuiSupport::layoutCurrentWindowAsFullCanvas();
    ImGui::Text("%s", _errorMessage.c_str());
    if (ImGui::Button("OK"))
    {
        _screen.store(Screen::Configuration);
        redrawNext();
    }
    ImGui::End();
}
