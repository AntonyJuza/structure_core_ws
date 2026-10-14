/*
    Layout.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Layout.h"
#include "Window.h"
#include <stdexcept>
#include <imgui.h>
#include <algorithm>

static bool operator==(const ImVec2& a, const ImVec2& b) {
    return a.x == b.x && a.y == b.y;
}

static bool operator!=(const ImVec2& a, const ImVec2& b) {
    return !(a == b);
}

void GuiSupport::insertBlankLine() {
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + ImGui::GetTextLineHeight());
}

void GuiSupport::layoutCurrentWindowAsFullCanvas() {
    unsigned width = Window::current().windowWidth();
    unsigned height = Window::current().windowHeight();
    ImVec2 newPos(0.f, 0.f);
    if (ImGui::GetWindowPos() != newPos) {
        ImGui::SetWindowPos(newPos);
    }
    ImVec2 newSize((float)width, (float)height);
    if (ImGui::GetWindowSize() != newSize) {
        ImGui::SetWindowSize(newSize);
    }
}

struct GridDimensions {
    unsigned cellAreaX, cellAreaY;
    unsigned cellAreaW, cellAreaH;
    unsigned cellW, cellH;
    unsigned toolAreaX, toolAreaY;
    unsigned toolAreaW, toolAreaH;
    unsigned toolW, toolH;
};

static void computeGridDimensions(const GuiSupport::GridConfig& cfg, GridDimensions *dim) {
    unsigned windowWidth = GuiSupport::Window::current().windowWidth();
    unsigned windowHeight = GuiSupport::Window::current().windowHeight();
    unsigned toolAreaWidth = std::min(windowWidth, cfg.toolAreaWidth);
    unsigned cellAreaWidth = windowWidth - std::min(toolAreaWidth, windowWidth);
    dim->cellAreaX = toolAreaWidth;
    dim->cellAreaY = 0;
    dim->cellAreaW = cellAreaWidth;
    dim->cellAreaH = windowHeight;
    dim->cellW = cellAreaWidth / std::max(1u, cfg.numCellsX);
    dim->cellH = windowHeight / std::max(1u, cfg.numCellsY);
    dim->toolAreaX = 0;
    dim->toolAreaY = 0;
    dim->toolAreaW = toolAreaWidth;
    dim->toolAreaH = windowHeight;
    dim->toolW = toolAreaWidth;
    dim->toolH = windowHeight / std::max(1u, cfg.numTools);
}

void GuiSupport::layoutCurrentWindowAsGridCell(const GridConfig& cfg, unsigned cellX, unsigned cellY, unsigned spanX, unsigned spanY) {
    if (cellX + spanX > cfg.numCellsX || cellY + spanY > cfg.numCellsY) {
        throw std::runtime_error("Cell position out of range for config");
    }
    GridDimensions dim;
    computeGridDimensions(cfg, &dim);
    ImVec2 newPos(float(dim.cellAreaX + cellX * dim.cellW), float(dim.cellAreaY + cellY * dim.cellH));
    ImVec2 newSize(float(spanX * dim.cellW), float(spanY * dim.cellH));
    if (ImGui::GetWindowPos() != newPos) {
        ImGui::SetWindowPos(newPos);
    }
    if (ImGui::GetWindowSize() != newSize) {
        ImGui::SetWindowSize(newSize);
    }
}

void GuiSupport::layoutCurrentWindowAsGridTool(const GridConfig& cfg, unsigned toolN) {
    if (toolN >= cfg.numTools) {
        throw std::runtime_error("Tool number out of range for config");
    }
    GridDimensions dim;
    computeGridDimensions(cfg, &dim);
    ImVec2 newPos(float(dim.toolAreaX), float(dim.toolAreaY + toolN * dim.toolH));
    ImVec2 newSize(ImVec2(float(dim.toolW), float(dim.toolH)));
    if (ImGui::GetWindowPos() != newPos) {
        ImGui::SetWindowPos(newPos);
    }
    if (ImGui::GetWindowSize() != newSize) {
        ImGui::SetWindowSize(newSize);
    }
}
