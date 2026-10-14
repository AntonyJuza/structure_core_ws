/*
    CaptureSessionDelegate.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include "MultiRecorder.h"

SessionDelegate::SessionDelegate(Window *owner) : _owner(owner) {}
SessionDelegate::~SessionDelegate()
{
    for (PerDeviceData* d : _deviceData)
    {
        delete d;
    }
    
    _deviceData.clear();
}

void SessionDelegate::captureSessionEventDidOccur(ST::CaptureSession* session, ST::CaptureSessionEventId event)
{
    GuiSupport::log("Capture session event: %s", ST::CaptureSessionSample::toString(event));

    // Wake the window's event loop so it can display the capture session status
    _owner->wake();
    switch (event)
    {
        case ST::CaptureSessionEventId::Detected:
        {
            // store session
            PerDeviceData* data = new PerDeviceData();
            data->parent = session;
            data->lastCaptureSessionEvent.store(event);
            _deviceData.push_back(data);

            GuiSupport::log("Added newly detected sensor!");

            break;
        }
        case ST::CaptureSessionEventId::ProdDataCorrupt: // it'll still stream
        // case ST::CaptureSessionEventId::Ready:
        case ST::CaptureSessionEventId::Connected:
        {
            _owner->captureSessionReadyToStream();
            getDeviceDataForSession(session)->lastCaptureSessionEvent.store(event);
            break;
        }
        case ST::CaptureSessionEventId::Streaming:
        {
            _owner->captureSessionIsStreaming();
            getDeviceDataForSession(session)->lastCaptureSessionEvent.store(event);
            break;
        }
        default:
            break;
    }
}

void SessionDelegate::captureSessionDidOutputSample(ST::CaptureSession* session, const ST::CaptureSessionSample& sample)
{
    PerDeviceData* data = getDeviceDataForSession(session);
    if (!data)
    {
        GuiSupport::log("getDeviceDataForSession returned nullptr.");
        return;
    }

    std::unique_lock<std::mutex> u(data->sampleLock);
    {
        if (data->occWriter.isWriting())
        {
            data->occWriter.writeCaptureSample(sample);
        }

        if (sample.depthFrame.isValid())
        {
            data->lastDepthFrame = sample.depthFrame;
            data->depthRateMonitor.tick();
        }

        if (sample.infraredFrame.isValid())
        {
            data->lastInfraredFrame = sample.infraredFrame;
            data->infraredRateMonitor.tick();
        }

        if (sample.visibleFrame.isValid())
        {
            data->lastVisibleOrColorFrame = sample.visibleFrame;
        }
    }
    u.unlock();

    // Wake the window's event loop so it can display the latest samples
    _owner->wake();
}

std::vector<PerDeviceData*>& SessionDelegate::getAllDeviceData()
{
    return _deviceData;
}

PerDeviceData* SessionDelegate::getDeviceDataForSession(ST::CaptureSession* session)
{
    for (PerDeviceData* d : _deviceData)
    {
        if (d->parent == session)
            return d;
    }
    
    return nullptr;
}
