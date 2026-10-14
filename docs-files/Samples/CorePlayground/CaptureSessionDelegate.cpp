/*
    CaptureSessionDelegate.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "CorePlayground.h"
using CorePlayground::Window;
using CorePlayground::SessionDelegate;

SessionDelegate::SessionDelegate(Window *owner) : _owner(owner) {}
SessionDelegate::~SessionDelegate()
{
    _owner->curCaptureSession().stopStreaming();
}

void SessionDelegate::captureSessionEventDidOccur(ST::CaptureSession *, ST::CaptureSessionEventId event) {
    GuiSupport::log("Capture session event: %s", ST::CaptureSessionSample::toString(event));
    _lastCaptureSessionEvent.store(event);
    // Wake the window's event loop so it can display the capture session status
    _owner->wake();
    switch (event) {
        case ST::CaptureSessionEventId::Connected:
            _owner->captureSessionReadyToStream();
            break;
        case ST::CaptureSessionEventId::Streaming:
            _owner->captureSessionIsStreaming();
            break;
        default:;
    }
}

void SessionDelegate::dynCalibrationEventDidOccur(ST::CaptureSession *, ST::DynamicCalibrationState state) {
    if(_lastDynCalbrationEvent != state){
        GuiSupport::log("Dynamic Calibration state: %s",  ST::CaptureSessionSample::toString(state));
        _lastDynCalbrationEvent.store(state);
        // Wake the window's event loop so it can display the capture session status
        _owner->wake();
    }
}

void SessionDelegate::captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample& sample) {
    std::unique_lock<std::mutex> u(_sampleLock);
    // Depth/visible/infrared frames can arrive individually or as part of a SynchronizedFrames sample

    if (_occWriter.isWriting()) {
        _occWriter.writeCaptureSample(sample);
    }

    if (sample.depthFrame.isValid()) {
        _lastDepthFrame = sample.depthFrame;
        _depthRateMonitor.tick();
    }
    if (sample.visibleFrame.isValid()) {
        _lastVisibleFrame = sample.visibleFrame;
        _visibleRateMonitor.tick();
    }
    if (sample.infraredFrame.isValid()) {
        _lastInfraredFrame = sample.infraredFrame;
        _infraredRateMonitor.tick();
    }
    if (sample.type == ST::CaptureSessionSample::Type::AccelerometerEvent) {
        _lastAccelerometerEvent = sample.accelerometerEvent;
        _accelerometerRateMonitor.tick();
    }
    if (sample.type == ST::CaptureSessionSample::Type::GyroscopeEvent) {
        _lastGyroscopeEvent = sample.gyroscopeEvent;
        _gyroscopeRateMonitor.tick();
    }
    u.unlock();
    // Wake the window's event loop so it can display the latest samples
    _owner->wake();
}

ST::CaptureSessionEventId SessionDelegate::lastCaptureSessionEvent() {
    return _lastCaptureSessionEvent.load();
}

ST::DynamicCalibrationState SessionDelegate::lastDynCalibrationState() {
    return _lastDynCalbrationEvent.load();
}

ST::DepthFrame SessionDelegate::lastDepthFrame() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastDepthFrame;
}

ST::ColorFrame SessionDelegate::lastVisibleFrame() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastVisibleFrame;
}

ST::InfraredFrame SessionDelegate::lastInfraredFrame() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastInfraredFrame;
}

ST::AccelerometerEvent SessionDelegate::lastAccelerometerEvent() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastAccelerometerEvent;
}

ST::GyroscopeEvent SessionDelegate::lastGyroscopeEvent() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _lastGyroscopeEvent;
}

double SessionDelegate::depthRate() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _depthRateMonitor.rate();
}

double SessionDelegate::visibleRate() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _visibleRateMonitor.rate();
}

double SessionDelegate::infraredRate() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _infraredRateMonitor.rate();
}

double SessionDelegate::accelerometerRate() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _accelerometerRateMonitor.rate();
}

double SessionDelegate::gyroscopeRate() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _gyroscopeRateMonitor.rate();
}

ST::OCCFileWriter& SessionDelegate::occFileWriter() {
    std::unique_lock<std::mutex> u(_sampleLock);
    return _occWriter;
}
