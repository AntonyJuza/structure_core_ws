/*
    FileSelector.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "FileSelector.h"
#include "Layout.h"
#include "Window.h"
#include "Style.h"
#include "Utility.h"
#include <imgui.h>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>
using FileSelector = GuiSupport::FileSelector;

#if _WIN32

#define WIN32_LEAN_AND_MEAN 1
#define VC_EXTRALEAN 1
#define NOMINMAX 1
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd

static const char pathSeparator[] = "\\";

static std::string defaultPath() {
    const char *home = getenv("USERPROFILE");
    if (!home) {
        throw std::runtime_error("USERPROFILE not defined in environment");
    }
    return std::string(home);
}

static std::vector<std::string> listDirectory(const std::string& path) {
    std::vector<std::string> files;
    WIN32_FIND_DATA fdat;
    HANDLE hdl = FindFirstFile((path + "\\*").c_str(), &fdat);
    if (hdl == INVALID_HANDLE_VALUE) {
        GuiSupport::log("Failed to list directory: %s", path.c_str());
    }
    else {
        do {
            if (!strcmp(fdat.cFileName, ".") || !strcmp(fdat.cFileName, "..")) {
                continue;
            }
            files.push_back(std::string(fdat.cFileName));
        } while (FindNextFile(hdl, &fdat));
        FindClose(hdl);
    }
    return files;
}

static bool isDirectory(const std::string& path) {
    DWORD attr = GetFileAttributes(path.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        GuiSupport::log("Failed to stat path: %s", path.c_str());
        return false;
    }
    else {
        return attr & FILE_ATTRIBUTE_DIRECTORY;
    }
}

static std::string joinPath(const std::vector<std::string>& parts) {
    std::ostringstream ss;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i != 0) {
            ss << pathSeparator;
        }
        ss << parts[i];
    }
    return ss.str();
}

#else

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static const char pathSeparator[] = "/";

#if __ANDROID__
static std::string defaultPath() {
    return std::string("/sdcard");
}
#else
static std::string defaultPath() {
    const char *home = getenv("HOME");
    if (!home) {
        throw std::runtime_error("HOME not defined in environment");
    }
    return std::string(home);
}
#endif

static std::vector<std::string> listDirectory(const std::string& path) {
    std::vector<std::string> files;
    DIR *dir = opendir(path.c_str());
    if (dir) {
        struct dirent *ent = nullptr;
        while ((ent = readdir(dir))) {
            if (!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, "..")) {
                continue;
            }
            files.push_back(std::string(ent->d_name));
        }
        closedir(dir);
    }
    else {
        GuiSupport::log("Failed to list directory '%s': %s", path.c_str(), strerror(errno));
    }
    return files;
}

static bool isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    else {
        GuiSupport::log("Failed to stat path '%s': %s", path.c_str(), strerror(errno));
        return false;
    }
}

static std::string joinPath(const std::vector<std::string>& parts) {
    if (parts.empty()) {
        return pathSeparator;
    }
    else {
        std::ostringstream ss;
        for (const auto& part : parts) {
            ss << pathSeparator << part;
        }
        return ss.str();
    }
}

#endif

static std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    for (size_t i = 0; i < path.size();) {
        size_t j = path.find(pathSeparator, i);
        if (j == std::string::npos) {
            parts.push_back(path.substr(i));
            break;
        }
        else if (j != i) {
            parts.push_back(path.substr(i, j - i));
        }
        i = j + 1;
    }
    return parts;
}

FileSelector::FileSelector(Mode mode, std::string initialPath, std::string initialFilename) : _mode(mode) {
    _pathParts = splitPath(initialPath.empty() ? defaultPath() : initialPath);
    memset(_saveFilenameBuf, 0, sizeof _saveFilenameBuf);
    memcpy(_saveFilenameBuf, initialFilename.c_str(), std::min(initialFilename.size(), sizeof _saveFilenameBuf - 1));
}

FileSelector::~FileSelector() {}

static bool atRootDirectory(const std::vector<std::string>& parts) {
#if _WIN32
    return parts.size() <= 1;
#else
    return parts.empty();
#endif
}

void FileSelector::renderInterior() {
    if (_result != Result::InProgress) {
        return;
    }

    if (ImGui::Button("Cancel")) {
        _result = Result::Cancelled;
    }
    ImGui::SameLine();
    if (atRootDirectory(_pathParts)) {
        GuiSupport::pushButtonDisabledStyle();
        ImGui::Button("Up");
        GuiSupport::popButtonDisabledStyle();
    }
    else if (ImGui::Button("Up")) {
        _pathParts.pop_back();
        _fileList = nullptr;
        GuiSupport::Window::current().redrawNext();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        _fileList = nullptr;
        GuiSupport::Window::current().redrawNext();
    }
    ImGui::Text("Current directory: %s", joinPath(_pathParts).c_str());
    if (_mode == Mode::SaveNewFile) {
        ImGui::Text("Filename:");
        ImGui::SameLine();
        ImGui::PushItemWidth(300);
        ImGui::InputText("##filename", _saveFilenameBuf, sizeof _saveFilenameBuf - 1);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (strlen(_saveFilenameBuf) == 0) {
            GuiSupport::pushButtonDisabledStyle();
            ImGui::Button("Save");
            GuiSupport::popButtonDisabledStyle();
        }
        else {
            if (ImGui::Button("Save")) {
                std::vector<std::string> fullPath = _pathParts;
                fullPath.push_back(std::string(_saveFilenameBuf));
                _result = Result::Complete;
                _resultPath = joinPath(fullPath);
            }
        }
    }

    if (!_fileList) {
        _fileList = std::make_unique<std::vector<FileInfo>>();
        std::vector<std::string> files = listDirectory(joinPath(_pathParts));
        std::sort(files.begin(), files.end());
        for (auto& file : files) {
            std::vector<std::string> fullPath = _pathParts;
            fullPath.push_back(file);
            _fileList->emplace_back(file, isDirectory(joinPath(fullPath)));
        }
    }

    ImGui::SetNextWindowPos(ImGui::GetCursorPos());
    ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.15f, 0.15f, 0.15f, 1.f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.f, 0.5f));
    ImGui::BeginChild("_FileSelector_files");
    for (const auto& file : *_fileList) {
        auto label = formatString("%s%s##_FileSelector_%p", file.name.c_str(), file.isDirectory ? pathSeparator : "", (void *)&file);
        if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvailWidth(), 16.f))) {
            std::vector<std::string> fullPath = _pathParts;
            fullPath.push_back(file.name);
            if (isDirectory(joinPath(fullPath))) {
                _pathParts = fullPath;
                _fileList = nullptr;
                GuiSupport::Window::current().redrawNext();
            }
            else {
                _result = Result::Complete;
                _resultPath = joinPath(fullPath);
            }
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(4);
}

FileSelector::Result FileSelector::result() {
    return _result;
}

std::string FileSelector::resultPath() {
    return _resultPath;
}
