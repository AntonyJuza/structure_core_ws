/*
    Style.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Style.h"
#include <imgui.h>

static ImVec4 _buttonDisabledColor(1.f, 0.f, 1.f, 1.f);

void GuiSupport::setButtonDisabledColor(const ImVec4& color) {
    _buttonDisabledColor = color;
}

void GuiSupport::pushButtonDisabledStyle() {
    ImGui::PushStyleColor(ImGuiCol_Button, _buttonDisabledColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _buttonDisabledColor);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, _buttonDisabledColor);
}

void GuiSupport::popButtonDisabledStyle() {
    ImGui::PopStyleColor(3);
}
