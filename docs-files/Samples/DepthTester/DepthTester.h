/*
    DepthTester.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#pragma once

#include <GuiSupport.h>
#include <SampleProcessing.h>

#include <ST/CaptureSession.h>
#include <ST/DepthImageGenerator.h>
#include <ST/OCCFileWriter.h>
#include <ST/Utilities.h>

#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <stdint.h>

namespace DepthTester
{
    class Window;

    class SessionDelegate : public ST::CaptureSessionDelegate
    {
    private:
        Window *const _owner;

        // Accessed by streaming screen and event callback
        std::atomic<ST::CaptureSessionEventId> _lastCaptureSessionEvent {ST::CaptureSessionEventId::Disconnected};

        /*
          The mutex synchronizes access to the stored CaptureSession samples and
          rate monitors, since they may be accessed from:

          - The GL rendering thread when the owning Window retrieves them to display
            in the user interface
          - The CaptureSession's internal frame delivery thread(s)
          - The CaptureSession's dedicated IMU sample delivery thread if
            CaptureSessionSettings::lowLatencyIMU is enabled (default)
        */
        std::mutex _sampleLock;

        ST::DepthFrame _lastSensorDepthFrame;
        ST::DepthFrame _lastGeneratedDepthFrame;
        ST::InfraredFrame _lastInfraredFrame;

        GuiSupport::RateMonitor _depthRateMonitor;
        GuiSupport::RateMonitor _infraredRateMonitor;

        ST::DepthImageGenerator _depthGenerator;
        ST::OCCFileWriter _nu3000DepthOccWriter;
        ST::OCCFileWriter _generatedDepthOccWriter;

    public:
        explicit SessionDelegate(Window *owner);
        ~SessionDelegate();

        ST::CaptureSessionEventId lastCaptureSessionEvent();

        // Thread-safe accessors for the stored CaptureSession samples
        ST::DepthFrame lastSensorDepthFrame();
        ST::DepthFrame lastGeneratedDepthFrame();
        ST::InfraredFrame lastInfraredFrame();

        // Thread-sade accessors for other items
        ST::OCCFileWriter& occFileWriter(int idx);

        // Thread-safe accessors for rate monitors
        double depthRate();
        double infraredRate();

        // CaptureSession callbacks
        void captureSessionEventDidOccur(ST::CaptureSession *, ST::CaptureSessionEventId) override;
        void captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample&) override;
    };

    enum DEPTH_CAT{
        NU3000_DEPTH,
        HOST_PROCESSED_DEPTH,
    };
    class Window : public GuiSupport::Window
    {
    private:
        enum class Screen
        {
            Configuration,
            LoadConfiguration,
            SaveConfiguration,
            ConfigurationError,
            Streaming,
        };

        /*
          Current screen value may be accessed by:

          - GL rendering thread
          - Thread on which CaptureSession Ready event is delivered, to determine whether
            streaming should start
        */
        std::atomic<Screen> _screen {Screen::Configuration};

        // Used for LoadConfiguration/SaveConfiguration screens
        std::unique_ptr<GuiSupport::FileSelector> _fileSelector;

        // Displayed if loading or saving configuration failed
        std::string _errorMessage;

        ST::CaptureSession _captureSession;
        SessionDelegate _sessionDelegate;
        ST::CaptureSessionSettings _settings;

        uint32_t _playIconTextureId = 0;
        uint32_t _stopIconTextureId = 0;

        std::unique_ptr<SampleProcessing::FrameRenderer> _frameRenderer;

        // Timestamps used to determine whether frames have changed and need to be rendered
        double _lastRenderedSensorDepthTimestamp = NAN;
        double _lastRenderedInfraredTimestamp = NAN;

        // Textures for rendered frames
        uint32_t _sensorDepthFrameTextureId = 0;
        uint32_t _generatedDepthFrameTextureId = 0;
        uint32_t _infraredFrameTextureId = 0;

        ST::CaptureSessionSettings defaultCaptureSessionSettings()
        {
            ST::CaptureSessionSettings settings;
            settings.source = ST::CaptureSessionSourceId::StructureCore;
            settings.dispatcher = ST::CaptureSessionDispatcherId::BackgroundThread;
            settings.applyExpensiveCorrection = true;
            settings.frameSyncEnabled = true;
            settings.structureCore.depthEnabled = true;
            settings.structureCore.depthFramerate = 10.f;
            settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::VGA;
            settings.structureCore.infraredEnabled = true;
            settings.structureCore.infraredResolution = ST::StructureCoreInfraredResolution::SXGA;
            settings.structureCore.infraredFramerate = 10.f;
            settings.structureCore.visibleEnabled = false;
            settings.structureCore.accelerometerEnabled = false;
            settings.structureCore.gyroscopeEnabled = false;
            return settings;
        }

        void renderConfigurationScreen();
        void renderLoadConfigurationScreen();
        void renderSaveConfigurationScreen();
        void renderConfigurationErrorScreen();
        void renderStreamingScreen();

    protected:
        void setupRendering() override;
        void renderFrame() override;
        void teardownRendering() override;

    public:
        Window();
        ~Window() override;

        // Called when user presses the play button and switches to the Streaming screen
        void prepareForStreaming();

        // Called when user presses the stop button and switches to the Configuration screen
        void exitStreaming();

        // Called by the session delegate when the session is ready
        void captureSessionReadyToStream();

        // Called by the session delegate when the session is officially streaming
        void captureSessionIsStreaming();

        // Called on Android when the sensor file descriptor is provided
        void plugStructureCoreFileDescriptor(int fileDescriptor);
        
        // Return the current CaptureSession reference.
        ST::CaptureSession& curCaptureSession();
    };
}
