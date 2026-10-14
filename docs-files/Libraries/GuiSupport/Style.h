/*
    Style.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

struct ImVec4;

namespace GuiSupport {
    void setButtonDisabledColor(const ImVec4&);
    void pushButtonDisabledStyle();
    void popButtonDisabledStyle();
}
