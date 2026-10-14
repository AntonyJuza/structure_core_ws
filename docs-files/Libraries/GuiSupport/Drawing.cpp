/*
    Drawing.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include "Drawing.h"
#include <imgui.h>
#if __ANDROID__
#    include <GLES3/gl31.h>
#else
#    include <GL/glew.h>
#endif
#include <algorithm>

uint32_t GuiSupport::generateTextureFromRGBA8Data(const uint8_t *data, unsigned width, unsigned height) {
    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}

void GuiSupport::drawTextureInContentArea(uint32_t textureId) {
    GLint iwidth = 0, iheight = 0;
    glBindTexture(GL_TEXTURE_2D, textureId);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iwidth);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &iheight);
    glBindTexture(GL_TEXTURE_2D, 0);
    float width = iwidth;
    float height = iheight;
    float contentWidth = ImGui::GetContentRegionAvail().x;
    float contentHeight = ImGui::GetContentRegionAvail().y;
    float fitWidth = 0.f;
    float fitHeight = 0.f;
    float xOffset = 0.f;
    float yOffset = 0.f;
    if (width >= 1.f && height >= 1.f && contentWidth >= 1.f && contentHeight >= 1.f) {
        float minFit = std::max(width / contentWidth, height / contentHeight);
        fitWidth = width / minFit;
        fitHeight = height / minFit;
        xOffset = (contentWidth - fitWidth) / 2;
        yOffset = (contentHeight - fitHeight) / 2;
    }
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
    ImGui::Image((void *)(uintptr_t)textureId, ImVec2(fitWidth, fitHeight));
}
