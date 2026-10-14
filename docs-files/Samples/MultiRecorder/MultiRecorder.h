/*
    MultiRecorder.h

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#pragma once

#include <GuiSupport.h>
#include <SampleProcessing.h>

#include <ST/DeviceManager.h>
#include <ST/OCCFileWriter.h>
#include <ST/Utilities.h>

#include <atomic>
#include <mutex>
#include <vector>

class Window;

struct PerDeviceData
{
    std::atomic<ST::CaptureSessionEventId> lastCaptureSessionEvent { ST::CaptureSessionEventId::Disconnected };
    
    std::mutex sampleLock;
    
    ST::InfraredFrame lastInfraredFrame;
    ST::DepthFrame lastDepthFrame;
    ST::ColorFrame lastVisibleOrColorFrame;
    
    ST::CaptureSession* parent;
    ST::OCCFileWriter occWriter;

    std::unique_ptr<SampleProcessing::FrameRenderer> frameRenderer;
    
    GuiSupport::RateMonitor depthRateMonitor;
    GuiSupport::RateMonitor infraredRateMonitor;
};

class SessionDelegate : public ST::CaptureSessionDelegate
{
public:
    explicit SessionDelegate(Window* window);
    virtual ~SessionDelegate() override;
    
    virtual void captureSessionEventDidOccur(ST::CaptureSession* session, ST::CaptureSessionEventId newEvent) override;
    virtual void captureSessionDidOutputSample(ST::CaptureSession* session, const ST::CaptureSessionSample& sample) override;
    
    std::vector<PerDeviceData*>& getAllDeviceData();
    PerDeviceData* getDeviceDataForSession(ST::CaptureSession* session);

private:
    Window* const _owner;
    std::vector<PerDeviceData*> _deviceData;
};

class Window : public GuiSupport::Window
{
public:
    Window();
    virtual ~Window() override;

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
    
protected:
    virtual void setupRendering() override;
    virtual void renderFrame() override;
    virtual void teardownRendering() override;
    
    void renderConfigurationScreen();
    void renderStreamingScreen();
    
    ST::CaptureSessionSettings defaultCaptureSessionSettings() const
    {
        ST::CaptureSessionSettings settings;

        //FIXME before release: Changed the default settings for fit3d project temporarily
        settings.source = ST::CaptureSessionSourceId::StructureCore;
        settings.dispatcher = ST::CaptureSessionDispatcherId::BackgroundThread;
        settings.applyExpensiveCorrection = false;
        
        settings.structureCore.depthEnabled = true;
        settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::VGA;
        settings.structureCore.depthFramerate = 30.f;

        settings.structureCore.infraredEnabled = false;
		settings.structureCore.infraredFramerate = 30.f;

        settings.structureCore.visibleEnabled = false;
        settings.structureCore.visibleFramerate = 30.f;
        settings.structureCore.demosaicMethod = ST::StructureCoreDemosaicMethod::Bilinear;

        settings.structureCore.accelerometerEnabled = false;
        settings.structureCore.gyroscopeEnabled = false;
        
        settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Short;
        settings.structureCore.initialInfraredExposure = 0.020f;
        settings.structureCore.initialInfraredGain = 1;

        settings.frameSyncEnabled = true;
        
        return settings;
    }

private:
    enum class Screen
    {
        Configuration,
        Streaming,
    };
    
    std::atomic<Screen> _currentScreen { Screen::Configuration };
    
    ST::DeviceManager _deviceManager;
    ST::CaptureSessionSettings _settings;
    SessionDelegate _delegate;
    
    uint32_t _depthTextureId = 0;
    uint32_t _visibleOrColorTextureId = 0;
    
    double _lastDepthTimestamp = NAN;
    double _lastRenderedInfraredTimestamp = NAN;
    double _lastVisibleOrColorTimestamp = NAN;

    bool _recordingOcc = false;
};
