/*
    SCSession.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    Updated for ROS 2 Jazzy.
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_

#include <ST/CaptureSession.h>
#include <ST/CameraFrames.h>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace structure_core
{

class Converter;

struct IMUSamples
{
    ST::AccelerometerEvent accel;
    ST::GyroscopeEvent gyro;

    std::map<double, ST::AccelerometerEvent> accel_events;
    std::map<double, ST::GyroscopeEvent> gyro_events;

    int num_acc = 0;
    int num_gyro = 0;

    void newAccSample(const ST::AccelerometerEvent& x)
    {
        accel_events[x.timestamp()] = x;
        num_acc = static_cast<int>(accel_events.size());
    }

    void newGyroSample(const ST::GyroscopeEvent& x)
    {
        gyro_events[x.timestamp()] = x;
        num_gyro = static_cast<int>(gyro_events.size());
    }

    bool interpolateAccelValue()
    {
        if (gyro_events.size() < 2 || accel_events.size() < 2)
            return false;

        auto gyro_it = std::prev(gyro_events.end(), 2);

        auto it_right = accel_events.lower_bound(gyro_it->first);
        auto it_left = accel_events.upper_bound(gyro_it->first);

        if (it_left != accel_events.begin())
            it_left--;

        if (it_right != accel_events.end() && it_right != it_left && it_right->first > it_left->first)
        {
            auto t = (gyro_it->first - it_left->first) / (it_right->first - it_left->first);

            auto x_accel = linearInterpolate(t, it_left->second.acceleration().x, it_right->second.acceleration().x);
            auto y_accel = linearInterpolate(t, it_left->second.acceleration().y, it_right->second.acceleration().y);
            auto z_accel = linearInterpolate(t, it_left->second.acceleration().z, it_right->second.acceleration().z);

            const char* device_id = "StructureCore";
            accel.setAccelEvent(x_accel, y_accel, z_accel, gyro_it->first, device_id);
            gyro = gyro_it->second;

            gyro_events.erase(gyro_events.begin(), gyro_it);
            accel_events.erase(accel_events.begin(), it_left);

            num_acc = static_cast<int>(accel_events.size());
            num_gyro = static_cast<int>(gyro_events.size());

            return true;
        }

        return false;
    }

    inline double linearInterpolate(double t, const double lhs, const double rhs)
    {
        return lhs + t * (rhs - lhs);
    }
};

struct ImageProcSettings
{
    bool depth_apply_correction_ = false;
    bool depth_aligned_enable_ = false;
    bool rgbd_enable_ = false;
    bool depth_pcloud_enable_ = false;
    bool ir_both_cameras = false;
    bool ir_left_camera = false;
    bool ir_right_camera = false;
};

struct SessionDelegate : ST::CaptureSessionDelegate
{
    std::mutex lock;
    std::condition_variable cond;
    bool ready = false;
    bool is_streaming = false;
    IMUSamples imu_samples;
    ImageProcSettings img_proc_settings;
    std::vector<std::shared_ptr<Converter>> device_listeners;
    std::mutex session_mutex;

    void captureSessionEventDidOccur(ST::CaptureSession *, ST::CaptureSessionEventId event) override
    {
        printf("Received capture session event %d (%s)\n", static_cast<int>(event), ST::CaptureSessionSample::toString(event));
        switch (event)
        {
            case ST::CaptureSessionEventId::Connected:
            {
                std::unique_lock<std::mutex> u(lock);
                is_streaming = false;
                ready = true;
                cond.notify_all();
                break;
            }
            case ST::CaptureSessionEventId::Streaming:
            {
                std::unique_lock<std::mutex> u(lock);
                is_streaming = true;
                cond.notify_all();
                break;
            }
            case ST::CaptureSessionEventId::Disconnected:
            case ST::CaptureSessionEventId::EndOfFile:
            case ST::CaptureSessionEventId::Error:
            {
                std::unique_lock<std::mutex> u(lock);
                ready = false;
                is_streaming = false;
                cond.notify_all();
                break;
            }
            default:
            {
                printf("Event %d unhandled\n", static_cast<int>(event));
                break;
            }
        }
    }

    void captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample& sample) override
    {
        std::lock_guard<std::mutex> lck(session_mutex);
        switch (sample.type)
        {
            case ST::CaptureSessionSample::Type::DepthFrame:
                if (sample.depthFrame.isValid())
                {
                    if (img_proc_settings.depth_apply_correction_)
                    {
                        ST::DepthFrame depthFrame = sample.depthFrame;
                        depthFrame.applyExpensiveCorrection();
                        notifyDepthListeners(depthFrame);
                    }
                    else
                    {
                        notifyDepthListeners(sample.depthFrame);
                    }
                }
                break;
            case ST::CaptureSessionSample::Type::VisibleFrame:
                if (sample.visibleFrame.isValid())
                {
                    notifyColorListeners(sample.visibleFrame);
                }
                break;
            case ST::CaptureSessionSample::Type::InfraredFrame:
                if (sample.infraredFrame.isValid())
                {
                    notifyIRListeners(sample.infraredFrame);
                }
                break;
            case ST::CaptureSessionSample::Type::SynchronizedFrames:
            {
                if (sample.visibleFrame.isValid())
                {
                    notifyColorListeners(sample.visibleFrame);
                }
                ST::DepthFrame depthFrame;
                if (sample.depthFrame.isValid())
                {
                    if (img_proc_settings.depth_apply_correction_)
                    {
                        depthFrame = sample.depthFrame;
                        depthFrame.applyExpensiveCorrection();
                        notifyDepthListeners(depthFrame);
                    }
                    else
                    {
                        depthFrame = sample.depthFrame;
                        notifyDepthListeners(depthFrame);
                    }
                }
                if (sample.visibleFrame.isValid() && sample.depthFrame.isValid())
                {
                    notifyRGBDListeners(depthFrame, sample.visibleFrame);
                }
                if (sample.infraredFrame.isValid())
                {
                    notifyIRListeners(sample.infraredFrame);
                }
                break;
            }
            case ST::CaptureSessionSample::Type::AccelerometerEvent:
                imu_samples.newAccSample(sample.accelerometerEvent);
                break;
            case ST::CaptureSessionSample::Type::GyroscopeEvent:
                imu_samples.newGyroSample(sample.gyroscopeEvent);
                break;
            default:
                break;
        }

        if (imu_samples.num_acc > 1 && imu_samples.num_gyro > 1)
        {
            if (imu_samples.interpolateAccelValue())
                notifyIMUListeners(imu_samples.accel, imu_samples.gyro);
        }
    }

    bool isStreaming()
    {
        std::unique_lock<std::mutex> u(lock);
        return is_streaming;
    }

    void waitUntilReady()
    {
        std::unique_lock<std::mutex> u(lock);
        cond.wait(u, [this]() {
            return ready;
        });
    }

    void attachListener(std::shared_ptr<Converter> deviceListener);
    void notifyIRListeners(const ST::InfraredFrame &infraredFrame);
    void notifyDepthListeners(const ST::DepthFrame &depthFrame);
    void notifyColorListeners(const ST::ColorFrame &visFrame);
    void notifyRGBDListeners(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame);
    void notifyIMUListeners(const ST::AccelerometerEvent &accel, const ST::GyroscopeEvent& gyro);
};

} // namespace structure_core

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_ */
