/*
    Apple.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/
#pragma once
#if !__APPLE__
#error "This header should only be included on macOS."
#endif

struct GLFWwindow;

namespace GuiSupport {
    void updateNSGLContextForWindow(struct GLFWwindow *window);
}
