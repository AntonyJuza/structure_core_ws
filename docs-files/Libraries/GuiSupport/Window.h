/*
    Window.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#pragma once
#include <memory>
#include <string>

namespace GuiSupport {
    class Window {
    public:
        struct Impl;

    private:
        std::unique_ptr<Impl> _impl;

    protected:
        /** Called within the GL context after ImGui has been initialized but
            before entering the event loop. */
        virtual void setupRendering() = 0;

        /** Render implementation called within the GL context for every
            frame. */
        virtual void renderFrame() = 0;

        /** Called within the GL context after exiting the event loop. */
        virtual void teardownRendering() = 0;

    public:
        Window();
        virtual ~Window();

        Window(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;

        /** Set initial window parameters before running the event loop. Ignored
            on Android. */
        void configureWindowBeforeCreation(unsigned initialWidth, unsigned initialHeight, bool maximize, const std::string& title);

        /** Wake the window's event loop as though user input occurred. Callable
            from any thread. No-op on Android since GLSurfaceView rendering is
            continuous. */
        void wake();

        /** Obtain a reference to the current window, or throw if not called
            within renderFrame(). */
        static Window& current();

        /** Force the next frame to redraw immediately instead of waiting for
            user input. Callable within renderFrame() only. No-op on Android
            since GLSurfaceView rendering is continuous. */
        void redrawNext();

        /** Return current window width. Callable within renderFrame() only. */
        unsigned windowWidth();

        /** Return current window height. Callable within renderFrame() only. */
        unsigned windowHeight();

#if __ANDROID__
        /** Render a frame within the current GL context. Current width and
            height can be obtained via GLSurfaceView.Renderer.onSurfaceChanged().
            Specify scaleFactor > 1.f for high DPI displays. */
        void renderFrameInGLSurfaceViewContext(unsigned currentWidth, unsigned currentHeight, float scaleFactor);

        /** Update ImGui mouse click state and coordinates. State can be
            obtained via GLSurfaceView.onTouchEvent(). */
        void updateMouseState(bool down, int x, int y);
#else
        /** Run the event loop until window closure. Blocking. */
        void runUntilWindowClosed();
#endif
    };
}
