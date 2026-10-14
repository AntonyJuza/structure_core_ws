/*
    STDevice.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDEVICE_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDEVICE_H_


#include <ST/CaptureSession.h>
#include <ST/CameraFrames.h>
#include <ST/Utilities.h>
#include <iostream>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <vector>

#include "SCtoSensorMsgConverter.h"
#include "SCSession.h"

typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image,  std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> ConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image,  std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info, geometry_msgs::msg::TransformStamped color_to_depth_tf)> DepthConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image,  std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> DepthAlignedConverterCallbackFunction;
typedef std::function<void(sensor_msgs::msg::Imu imu_msg)> ConverterIMUCallbackFunction;
typedef std::function<void(sensor_msgs::msg::PointCloud2 rgbd_points)> ConverterRGBDCallbackFunction;

namespace structure_core
{
	class CameraDevice
	{
	public:
		CameraDevice()
		{
			converter = std::make_shared<Converter>();
			delegate = std::unique_ptr<SessionDelegate>(new SessionDelegate());
			delegate->attachListener(converter);
		}

		int startStream()
		{
			startMonitoring();

			delegate->waitUntilReady();
			session.startStreaming();

			return 0;
		}

		void stopStream()
		{
			session.stopStreaming();
		}

		void startMonitoring()
		{
			session_initialized = true;
			session.setDelegate(delegate.get());
			if (!session.startMonitoring(settings))
			{
				printf("Failed to initialize capture session\n");
			}
		}

		void setColorFrameCallback(ConverterCallbackFunction callback)
		{
			converter->setColorCallback(callback);
		}

		void setDepthFrameCallback(DepthConverterCallbackFunction callback)
		{
			converter->setDepthCallback(callback);
		}

		void setDepthAlignedFrameCallback(DepthAlignedConverterCallbackFunction callback)
		{
			converter->setDepthAlignedCallback(callback);
		}

		void setDepthPointsCallback(ConverterDepthPointsCallbackFunction callback)
		{
			converter->setDepthPointsCallback(callback);
		}

		void setRGBDCallback(ConverterRGBDCallbackFunction callback)
		{
			converter->setRGBDCallback(callback);
		}

		void setIrLeftFrameCallback(ConverterCallbackFunction callback)
		{
			converter->setIrLeftCallback(callback);
		}

		void setIrRightFrameCallback(ConverterCallbackFunction callback)
		{
			converter->setIrRightCallback(callback);
		}
		void setIMUFrameCallback(ConverterIMUCallbackFunction callback)
		{
			converter->setIMUCallback(callback);
		}

		void setNamespace(std::string camera_name)
		{
			converter->setNamespace(camera_name);
		}

		void setParams( bool visible_enable_,
						bool visible_apply_gamma_correction_,
						float visible_framerate_,
						std::string visible_resolution_,
						bool infrared_enable_,
						float infrared_framerate_,
						std::string infrared_mode_,
						std::string infrared_resolution_,
						bool infrared_disable_intensity_balance_,
						bool imu_enable_,
						std::string imu_update_rate_,
						bool depth_enable_,
						bool depth_aligned_enable_,
						bool depth_pcloud_enable_,
						float depth_framerate_,
						std::string depth_resolution_,
						bool depth_apply_correction_before_stream_,
						std::string demosaic_method_,
						float initial_projector_power_,
						bool latency_reducer_enabled_,
						int sensor_initialization_timeout_,
						std::string sensor_serial_,
						bool frame_sync_enabled_,
						bool rgbd_enable_,
						float visible_initial_gain_,
						float visible_initial_exposure_,
						bool infrared_auto_exposure_enabled_,
						float infrared_initial_exposure_,
						float infrared_initial_gain_,
						bool depth_apply_correction_,
						int depth_range_mode_,
						int dynamic_calibration_mode_,
						std::string camera_name_)
		{

			settings.source = ST::CaptureSessionSourceId::StructureCore;

			settings.structureCore.visibleEnabled = visible_enable_;
			settings.structureCore.visibleApplyGammaCorrection = visible_apply_gamma_correction_;
			settings.structureCore.visibleFramerate = visible_framerate_;

			if(visible_resolution_ == "HowMany")
				settings.structureCore.visibleResolution = ST::StructureCoreVisibleResolution::HowMany;
			else
				settings.structureCore.visibleResolution = ST::StructureCoreVisibleResolution::Default;

			settings.structureCore.infraredEnabled = infrared_enable_;
			settings.structureCore.infraredFramerate = infrared_framerate_;

			if(infrared_mode_ == "HowMany")
            	settings.structureCore.infraredMode = ST::StructureCoreInfraredMode::HowMany;
			else if(infrared_mode_ == "BothCameras")
            	settings.structureCore.infraredMode = ST::StructureCoreInfraredMode::BothCameras;
			else if(infrared_mode_ == "RightCameraOnly")
                settings.structureCore.infraredMode = ST::StructureCoreInfraredMode::RightCameraOnly;
			else if(infrared_mode_ == "LeftCameraOnly")
                settings.structureCore.infraredMode = ST::StructureCoreInfraredMode::LeftCameraOnly;
			else
				settings.structureCore.infraredMode = ST::StructureCoreInfraredMode::Default;

			if(settings.structureCore.infraredMode == ST::StructureCoreInfraredMode::BothCameras)
				delegate->img_proc_settings.ir_both_cameras = true;
			else if(settings.structureCore.infraredMode == ST::StructureCoreInfraredMode::LeftCameraOnly)
				delegate->img_proc_settings.ir_left_camera = true;
			else if(settings.structureCore.infraredMode == ST::StructureCoreInfraredMode::RightCameraOnly)
				delegate->img_proc_settings.ir_right_camera = true;

			if(infrared_resolution_ == "HowMany")
				settings.structureCore.infraredResolution = ST::StructureCoreInfraredResolution::HowMany;
			else
				settings.structureCore.infraredResolution = ST::StructureCoreInfraredResolution::Default;

			settings.structureCore.disableInfraredIntensityBalance = infrared_disable_intensity_balance_;

			settings.structureCore.accelerometerEnabled = imu_enable_;
			settings.structureCore.gyroscopeEnabled = imu_enable_;

			if(imu_update_rate_ == "HowMany")
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::HowMany;
			else if(imu_update_rate_ == "AccelAndGyro_1000Hz")
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::AccelAndGyro_1000Hz;
			else if(imu_update_rate_ == "AccelAndGyro_400Hz")
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::AccelAndGyro_400Hz;
			else if(imu_update_rate_ == "AccelAndGyro_200Hz")
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::AccelAndGyro_200Hz;
			else if(imu_update_rate_ == "AccelAndGyro_100Hz")
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::AccelAndGyro_100Hz;
			else
				settings.structureCore.imuUpdateRate = ST::StructureCoreIMUUpdateRate::Default;

			settings.structureCore.depthEnabled = depth_enable_;
			settings.structureCore.depthFramerate = depth_framerate_;

			if(depth_resolution_ == "HowMany")
				settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::HowMany;
			else if(depth_resolution_ == "VGA" || depth_resolution_ == "_640x480")
				settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::VGA;
			else if(depth_resolution_ == "QVGA" || depth_resolution_ == "_320x240")
				settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::QVGA;
			else if(depth_resolution_ == "SXGA" || depth_resolution_ == "_1280x960")
				settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::SXGA;
			else
				settings.structureCore.depthResolution = ST::StructureCoreDepthResolution::Default;

			if(demosaic_method_ == "Bilinear")
				settings.structureCore.demosaicMethod = ST::StructureCoreDemosaicMethod::Bilinear;
			else if(demosaic_method_ == "EdgeAware")
				settings.structureCore.demosaicMethod = ST::StructureCoreDemosaicMethod::EdgeAware;
			else if(demosaic_method_ == "HowMany")
				settings.structureCore.demosaicMethod = ST::StructureCoreDemosaicMethod::HowMany;
			else
				settings.structureCore.demosaicMethod = ST::StructureCoreDemosaicMethod::Default;

			settings.applyExpensiveCorrection = depth_apply_correction_before_stream_;
			//settings.structureCore.initialProjectorPower = initial_projector_power_;
			settings.structureCore.latencyReducerEnabled = latency_reducer_enabled_;
			settings.structureCore.sensorInitializationTimeout = sensor_initialization_timeout_;

			settings.frameSyncEnabled = frame_sync_enabled_;

			if(rgbd_enable_ || depth_aligned_enable_)
			{
				settings.structureCore.visibleEnabled = true;
				settings.structureCore.depthEnabled = true;
			}

			delegate->img_proc_settings.rgbd_enable_= rgbd_enable_;
			delegate->img_proc_settings.depth_aligned_enable_= depth_aligned_enable_;
			delegate->img_proc_settings.depth_pcloud_enable_= depth_pcloud_enable_;

			settings.structureCore.initialVisibleExposure = visible_initial_exposure_;
			settings.structureCore.initialVisibleGain = visible_initial_gain_;

			settings.structureCore.initialInfraredExposure = infrared_initial_exposure_;
			settings.structureCore.initialInfraredGain = infrared_initial_gain_;

			delegate->img_proc_settings.depth_apply_correction_= depth_apply_correction_;

			setStaticAsDynParams(depth_range_mode_, dynamic_calibration_mode_, infrared_auto_exposure_enabled_);

			setNamespace(camera_name_);

		}

		bool setNewDynParams(float visible_initial_gain_,
						     float visible_initial_exposure_,
						     bool infrared_auto_exposure_enabled_,
						     float infrared_initial_exposure_,
						     float infrared_initial_gain_,
						     bool depth_apply_correction,
						     int depth_range_mode_,
						     int dynamic_calibration_mode_)
		{
			if(session_initialized)
			{
				if(visible_initial_gain_ != settings.structureCore.initialVisibleGain ||
						visible_initial_exposure_ != settings.structureCore.initialVisibleExposure ||
						infrared_initial_exposure_ != settings.structureCore.initialInfraredExposure	||
						infrared_initial_gain_ != settings.structureCore.initialInfraredGain ||
						depth_apply_correction != delegate->img_proc_settings.depth_apply_correction_)
					setDynParams(visible_initial_gain_, visible_initial_exposure_,
							     infrared_initial_exposure_, infrared_initial_gain_, depth_apply_correction);

				if(depth_range_mode_ != static_cast<int>(settings.structureCore.depthRangeMode) ||
				   dynamic_calibration_mode_ != static_cast<int>(settings.structureCore.dynamicCalibrationMode))
				{
					if(delegate->isStreaming())
					{
						stopStream();
						setStaticAsDynParams(depth_range_mode_, dynamic_calibration_mode_, infrared_auto_exposure_enabled_);
						startStream();
					}
					else
						setStaticAsDynParams(depth_range_mode_, dynamic_calibration_mode_, infrared_auto_exposure_enabled_);
				}
				return true;
			}
			return false;
		}

		void setDynParams(float visible_initial_gain_,
						  float visible_initial_exposure_,
						  float infrared_initial_exposure_,
						  float infrared_initial_gain_,
						  bool depth_apply_correction_)
		{
			if(delegate->isStreaming()){
				session.setVisibleCameraExposureAndGain(visible_initial_exposure_, visible_initial_gain_);
				session.setInfraredCamerasExposureAndGain(infrared_initial_exposure_, infrared_initial_gain_);

				delegate->img_proc_settings.depth_apply_correction_= depth_apply_correction_;
			}
		}

		void setStaticAsDynParams(int depth_range_mode_, int dynamic_calibration_mode_, bool infrared_auto_exposure_enabled_)
		{
			if(depth_range_mode_ == 0)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::VeryShort;
			else if(depth_range_mode_ == 1)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Short;
			else if(depth_range_mode_ == 2)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Medium;
			else if(depth_range_mode_ == 3)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Long;
			else if(depth_range_mode_ == 4)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::VeryLong;
			else if(depth_range_mode_ == 5)
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Hybrid;
			else
				settings.structureCore.depthRangeMode = ST::StructureCoreDepthRangeMode::Default;

			if(dynamic_calibration_mode_ == 1)
				settings.structureCore.dynamicCalibrationMode = ST::StructureCoreDynamicCalibrationMode::OneShotPersistent;
			else if(dynamic_calibration_mode_ == 2)
				settings.structureCore.dynamicCalibrationMode = ST::StructureCoreDynamicCalibrationMode::ContinuousNonPersistent;
			else
				settings.structureCore.dynamicCalibrationMode = ST::StructureCoreDynamicCalibrationMode::Off;

			settings.structureCore.infraredAutoExposureEnabled = infrared_auto_exposure_enabled_;
		}

		const bool sessionIsInit ()
		{
			return session_initialized;
		}

	private:
		std::unique_ptr<SessionDelegate> delegate;
		std::shared_ptr<Converter> converter;
		ST::CaptureSessionSettings settings;
		ST::CaptureSession session;
		std::mutex lock;
		bool session_initialized = false;

	};

}


#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDEVICE_H_ */
