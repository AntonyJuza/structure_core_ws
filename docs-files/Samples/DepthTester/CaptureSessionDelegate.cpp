/*
    CaptureSessionDelegate.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "DepthTester.h"

using DepthTester::Window;
using DepthTester::SessionDelegate;

SessionDelegate::SessionDelegate(Window *owner) : _owner(owner) {}
SessionDelegate::~SessionDelegate()
{
    _owner->curCaptureSession().stopStreaming();
}

void SessionDelegate::captureSessionEventDidOccur(ST::CaptureSession *, ST::CaptureSessionEventId event)
{
    GuiSupport::log("Capture session event: %s", ST::CaptureSessionSample::toString(event));
    _lastCaptureSessionEvent.store(event);
    // Wake the window's event loop so it can display the capture session status
    _owner->wake();
    switch (event)
    {
        case ST::CaptureSessionEventId::Connected:
            _depthGenerator.setMinAndMaxDepthInMm(400.f, 4000.f);
            _owner->captureSessionReadyToStream();
            break;
        case ST::CaptureSessionEventId::Streaming:
            _owner->captureSessionIsStreaming();
            break;
        default:;
    }
}

void SessionDelegate::captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample& sample)
{
    {
        std::unique_lock<std::mutex> u(_sampleLock);

        if (sample.depthFrame.isValid() && sample.infraredFrame.isValid())
        {
            _lastSensorDepthFrame = sample.depthFrame;
            _lastInfraredFrame = sample.infraredFrame;
            _depthRateMonitor.tick();
            _infraredRateMonitor.tick();
            
            _lastGeneratedDepthFrame = _depthGenerator.generateDepthFromSynchronizedDepthAndInfrared(
                _lastSensorDepthFrame, _lastInfraredFrame
            );
            if (_generatedDepthOccWriter.isWriting())
            {
                //_occWriter.writeGeneratedDepthFrame(sample.depthFrame);
                _generatedDepthOccWriter.writeDepthFrame(_lastGeneratedDepthFrame);
                //_occWriter.writeCaptureSample((const ST::CaptureSessionSample&)*retSample);
            }
            if (_nu3000DepthOccWriter.isWriting())
            {
                _nu3000DepthOccWriter.writeDepthFrame(_lastSensorDepthFrame);
            }
        }
    }

    // Wake the window's event loop so it can display the latest samples
    _owner->wake();
}

ST::CaptureSessionEventId SessionDelegate::lastCaptureSessionEvent()
{
    return _lastCaptureSessionEvent.load();
}

ST::DepthFrame SessionDelegate::lastSensorDepthFrame()
{
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastSensorDepthFrame;
}

ST::DepthFrame SessionDelegate::lastGeneratedDepthFrame()
{
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastGeneratedDepthFrame;
}

ST::InfraredFrame SessionDelegate::lastInfraredFrame()
{
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastInfraredFrame;
}

double SessionDelegate::depthRate()
{
    std::unique_lock<std::mutex> u(_sampleLock);
    return _depthRateMonitor.rate();
}

double SessionDelegate::infraredRate()
{
    std::unique_lock<std::mutex> u(_sampleLock);
    return _infraredRateMonitor.rate();
}

ST::OCCFileWriter& SessionDelegate::occFileWriter(int idx) {
    std::unique_lock<std::mutex> u(_sampleLock);
    if(idx == DepthTester::DEPTH_CAT::NU3000_DEPTH)
        return _nu3000DepthOccWriter;
    else
        return _generatedDepthOccWriter;
}

