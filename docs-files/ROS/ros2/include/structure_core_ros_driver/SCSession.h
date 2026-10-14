/*
    SCSession.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_

#include <ST/CaptureSession.h>
#include <ST/CameraFrames.h>
#include <unordered_map>
#include <map>
#include <vector>

namespace structure_core
{

struct IMUSamples
{
	ST::AccelerometerEvent accel;
	ST::GyroscopeEvent gyro;

	std::map<double, ST::AccelerometerEvent> accel_events;
	std::map<double, ST::GyroscopeEvent> gyro_events;
	std::vector<double> accel_timestamps;

	int num_acc = 0;
	int num_gyro = 0;

	void newAccSample(const ST::AccelerometerEvent& x)
	{
		accel_events[x.timestamp()] = x;
	    num_acc = accel_events.size();
	}

	void newGyroSample(const ST::GyroscopeEvent& x)
	{
		gyro_events[x.timestamp()] = x;
	    num_gyro = gyro_events.size();
	}

	bool interpolateAccelValue()
	{
		auto gyro_it =  std::prev(gyro_events.end(), 2);  //gyro_events.begin();

		auto it_right = accel_events.lower_bound(gyro_it->first);
		auto it_left = accel_events.upper_bound(gyro_it->first);

		if (it_left != accel_events.begin())
			it_left--;

		if ( it_right != accel_events.end() && it_right != it_left)
		{
			auto t = (gyro_it->first - it_left->first) / (it_right->first - it_left->first);

			auto x_accel = linearInterpolate(t, it_left->second.acceleration().x, it_right->second.acceleration().x);
			auto y_accel = linearInterpolate(t, it_left->second.acceleration().y, it_right->second.acceleration().y);
			auto z_accel = linearInterpolate(t, it_left->second.acceleration().z, it_right->second.acceleration().z);

			const char* device_id = "StructureCore";
			accel.setAccelEvent(x_accel, y_accel, z_accel, gyro_it->first, (const char *)device_id);
			gyro = gyro_it->second;

			gyro_events.erase(gyro_events.begin(), gyro_it);
			accel_events.erase(accel_events.begin(), it_left);

			num_acc = accel_events.size();
			num_gyro = gyro_events.size();

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

struct SessionDelegate : ST::CaptureSessionDelegate {
    std::mutex lock;
    std::condition_variable cond;
    bool ready = false;
    IMUSamples imu_samples;
    ImageProcSettings img_proc_settings;

    void captureSessionEventDidOccur(ST::CaptureSession *, ST::CaptureSessionEventId event) override {
        printf("Received capture session event %d (%s)\n", (int)event, ST::CaptureSessionSample::toString(event));
        switch (event) {
            case ST::CaptureSessionEventId::Connected: {
                std::unique_lock<std::mutex> u(lock);
                is_streaming = false;
                ready = true;
                cond.notify_all();
            } break;
            case ST::CaptureSessionEventId::Streaming:{
                std::unique_lock<std::mutex> u(lock);
            	is_streaming = true;
            	break;
            }
            case ST::CaptureSessionEventId::Disconnected:
            case ST::CaptureSessionEventId::EndOfFile:
            case ST::CaptureSessionEventId::Error: {
                std::unique_lock<std::mutex> u(lock);
                ready = false;
            	is_streaming = false;
                cond.notify_all();
            } break;
            default:{
                printf("Event %d unhandled\n", (int)event);
                break;
            }
        }
    }

    void captureSessionDidOutputSample(ST::CaptureSession *, const ST::CaptureSessionSample& sample) {
    	std::lock_guard<std::mutex> lock(session_mutex);
    	switch (sample.type) {
            case ST::CaptureSessionSample::Type::DepthFrame:
				if(sample.depthFrame.isValid())
				{
					if(img_proc_settings.depth_apply_correction_)
					{
						ST::DepthFrame depthFrame = sample.depthFrame;
						depthFrame.applyExpensiveCorrection();
						notifyDepthListeners(depthFrame);
					}
					else
						notifyDepthListeners(sample.depthFrame);
				}
                break;
            case ST::CaptureSessionSample::Type::VisibleFrame:
                if(sample.visibleFrame.isValid())
                {
                	notifyColorListeners(sample.visibleFrame);
                }
                break;
            case ST::CaptureSessionSample::Type::InfraredFrame:
                if(sample.infraredFrame.isValid())
                {
                	notifyIRListeners(sample.infraredFrame);
                }
                break;
            case ST::CaptureSessionSample::Type::SynchronizedFrames:
            {
				if(sample.visibleFrame.isValid())
				{
					notifyColorListeners(sample.visibleFrame);
				}
				ST::DepthFrame depthFrame;
				if(sample.depthFrame.isValid())
				{
					if(img_proc_settings.depth_apply_correction_)
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
				if(sample.visibleFrame.isValid() && sample.depthFrame.isValid())
				{
					notifyRGBDListeners(depthFrame, sample.visibleFrame);
				}
				if(sample.infraredFrame.isValid())
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
                printf("Sample type %d unhandled\n", (int)sample.type);
        }

    	if(imu_samples.num_acc > 1 && imu_samples.num_gyro > 1)
    	{
    		if(imu_samples.interpolateAccelValue())
    			notifyIMUListeners(imu_samples.accel, imu_samples.gyro);
    	}
    }

    bool isStreaming()
    {
    	bool ret = false;
        std::unique_lock<std::mutex> u(lock);
        ret = is_streaming;
    	return ret;
    }
    void waitUntilReady()
    {
        std::unique_lock<std::mutex> u(lock);
        cond.wait(u, [this]() {
            return ready;
        });
    }


  	void attachListener(std::shared_ptr<Converter> deviceListener)
	{
    	device_listeners.push_back(deviceListener);
  	};

  	void notifyIRListeners(const ST::InfraredFrame &infraredFrame)
  	{
  	 	for (int i= 0; i < device_listeners.size(); i++)
  	 	{
  	 		if(img_proc_settings.ir_both_cameras)
  	 			device_listeners[i]->onIRBothFramesAvailableEvent(infraredFrame);
  	 		else
  	 			device_listeners[i]->onIROneFrameAvailableEvent(infraredFrame, img_proc_settings.ir_left_camera);
     	}
    }

    void notifyDepthListeners(const ST::DepthFrame &depthFrame)
  	{
  	 	for (int i= 0; i < device_listeners.size(); i++)
  	 	{
        	device_listeners[i]->onDepthFrameAvailableEvent(depthFrame);
          if(img_proc_settings.depth_pcloud_enable_)
            device_listeners[i]->onDepthPointsAvailableEvent(depthFrame);
     	}
    }

    void notifyColorListeners(const ST::ColorFrame &visFrame)
  	{
  	 	for (int i= 0; i < device_listeners.size(); i++)
  	 	{
        	device_listeners[i]->onColorFrameAvailableEvent(visFrame);
     	}
    }

    void notifyRGBDListeners(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
    {
      for (int i= 0; i < device_listeners.size(); i++)
      {
        if(img_proc_settings.rgbd_enable_)
          device_listeners[i]->onRGBDAvailableEvent(depthFrame, visFrame);
        if(img_proc_settings.depth_aligned_enable_)
          device_listeners[i]->onDepthAlignedAvailableEvent(depthFrame, visFrame);
      }
    }

    void notifyIMUListeners(const ST::AccelerometerEvent &accel, const ST::GyroscopeEvent& gyro)
  	{
  	 	for (int i= 0; i < device_listeners.size(); i++)
  	 	{
        	device_listeners[i]->onIMUAvailableEvent(accel, gyro);
     	}
    }

private:
	std::vector<std::shared_ptr<Converter> > device_listeners;
	std::mutex session_mutex;
	bool is_streaming = false;
};

}


#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCSESSION_H_ */
