/*
    SimpleStreamer.cpp

    Copyright © 2020 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
 */

#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#if __APPLE__
#    include <AppKit/AppKit.h>
#endif

#include <ST/CaptureSession.h>

struct SessionDelegate : ST::CaptureSessionDelegate {
    void captureSessionEventDidOccur(ST::CaptureSession *session, ST::CaptureSessionEventId event) override {
        printf("Received capture session event %d (%s)\n", (int)event, ST::CaptureSessionSample::toString(event));
        switch (event) {
            case ST::CaptureSessionEventId::Booting: break;
            case ST::CaptureSessionEventId::Connected:
                printf("Starting streams...\n");
                printf("Sensor Serial Number is %s \n ", session->sensorInfo().serialNumber);
                session->startStreaming();
                break;
            case ST::CaptureSessionEventId::Disconnected:
            case ST::CaptureSessionEventId::Error:
                printf("Capture session error\n");
                exit(1);
                break;
            default:
                printf("Capture session event unhandled\n");
        }
    }

    void captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample& sample) override {
        printf("Received capture session sample of type %d (%s)\n", (int)sample.type, ST::CaptureSessionSample::toString(sample.type));
        switch (sample.type) {
            case ST::CaptureSessionSample::Type::DepthFrame:
                printf("Depth frame: size %dx%d\n", sample.depthFrame.width(), sample.depthFrame.height());
                break;
            case ST::CaptureSessionSample::Type::VisibleFrame:
                printf("Visible frame: size %dx%d\n", sample.visibleFrame.width(), sample.visibleFrame.height());
                break;
            case ST::CaptureSessionSample::Type::InfraredFrame:
                printf("Infrared frame: size %dx%d\n", sample.infraredFrame.width(), sample.infraredFrame.height());
                break;
            case ST::CaptureSessionSample::Type::SynchronizedFrames:
                printf("Synchronized frames: depth %dx%d visible %dx%d infrared %dx%d\n", sample.depthFrame.width(), sample.depthFrame.height(), sample.visibleFrame.width(), sample.visibleFrame.height(), sample.infraredFrame.width(), sample.infraredFrame.height());
                break;
            case ST::CaptureSessionSample::Type::AccelerometerEvent:
                printf("Accelerometer event: [% .5f % .5f % .5f]\n", sample.accelerometerEvent.acceleration().x, sample.accelerometerEvent.acceleration().y, sample.accelerometerEvent.acceleration().z);
                break;
            case ST::CaptureSessionSample::Type::GyroscopeEvent:
                printf("Gyroscope event: [% .5f % .5f % .5f]\n", sample.gyroscopeEvent.rotationRate().x, sample.gyroscopeEvent.rotationRate().y, sample.gyroscopeEvent.rotationRate().z);
                break;
            default:
                printf("Sample type unhandled\n");
        }
    }
};

void run() {
    ST::CaptureSessionSettings settings;
    settings.source = ST::CaptureSessionSourceId::StructureCore;
    settings.structureCore.depthEnabled = true;
    settings.structureCore.visibleEnabled = true;
    settings.structureCore.infraredEnabled = true;
    settings.structureCore.accelerometerEnabled = true;
    settings.structureCore.gyroscopeEnabled = true;
    settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::VGA;
    settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::AccelAndGyro_200Hz;

    SessionDelegate delegate;
    ST::CaptureSession session;
    session.setDelegate(&delegate);
    if (!session.startMonitoring(settings)) {
        printf("Failed to initialize capture session!\n");
        exit(1);
    }

    /* Loop forever. The SessionDelegate receives samples on a background thread
       while streaming. */
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

#if __APPLE__

// On macOS, a run loop is required for the Structure Core driver to function.

@interface AppDelegate : NSObject<NSApplicationDelegate>
@property int argc;
@property char **argv;
@end
@implementation AppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notif {
    // Launch run() on a separate thread to avoid blocking the main run loop
    std::thread(run).detach();
}
@end

int main(void) {
    @autoreleasepool {
        AppDelegate *delegate = [AppDelegate new];
        NSApplication *app = [NSApplication sharedApplication];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}

#else

int main(void) {
    run();
    return 0;
}

#endif
