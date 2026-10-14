/*
    AppInterface.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once

/** These functions define the interface between cross-platform sample
    application code and platform-specific wrappers. */
namespace AppInterface {
    /** On desktop platforms this function is called in main() before
        runUntilWindowClosed(). On Android it is called when the main activity
        is created. */
    void setup();

    /** On desktop platforms this function is called in main() after
        runUntilWindowClosed(). On Android it is called when the main activity
        is destroyed. */
    void teardown();

#if __ANDROID__
    /** See Window::renderFrameInGLSurfaceViewContext(). */
    void renderFrame(unsigned currentWidth, unsigned currentHeight, float scaleFactor);

    /** See Window::updateMouseState(). */
    void updateMouseState(bool down, int x, int y);

    /** For applications that require Structure Core USB support. The argument
        is a file descriptor from the Android UsbDeviceConnection API and should
        be passed to ST::registerSensorByUSBFileDescriptor() or equivalent. */
    void plugStructureCoreFileDescriptor(int fd);
#else
    /** Called in main(). */
    void runUntilWindowClosed();
#endif
    /** For applications requiring CLI command support, this function will be processed before getting any GUI level setup*/
    void cliCmdHandler(int argc, char **argv);
}
