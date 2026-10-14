/*
    Drawing.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once
#include <stdint.h>

namespace GuiSupport {
    uint32_t generateTextureFromRGBA8Data(const uint8_t *data, unsigned width, unsigned height);
    void drawTextureInContentArea(uint32_t textureId);
}
