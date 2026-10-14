/*
    SCDriver.cpp

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#include <condition_variable>
#include <mutex>
#include <functional>
#include <memory>
#include "structure_core_ros_driver/SCDriver.h"

namespace structure_core
{

	Driver::Driver() : Nodelet() {}

	Driver::~Driver() {}

	void Driver::onInit()
	{
		nh = getMTNodeHandle();
		pnh = getMTPrivateNodeHandle();

		device = std::make_shared<structure_core::CameraDevice>();

		readParams();

		std::lock_guard<std::mutex> lock(connect_mutex_);

		if(visible_enable_)
		{
			ros::NodeHandle color_image_nh(nh, name_ + "/rgb");
			image_transport::ImageTransport color_image_it(color_image_nh);

			image_transport::SubscriberStatusCallback itssc_color = std::bind(&Driver::connectCb, this);
			ros::SubscriberStatusCallback rssc_color = std::bind(&Driver::connectCb, this);
			pub_color = color_image_it.advertiseCamera("image", 1, itssc_color, itssc_color, rssc_color, rssc_color);
		}

		if(depth_enable_)
		{
			ros::NodeHandle depth_image_nh(nh, name_ + "/depth");
			image_transport::ImageTransport depth_image_it(depth_image_nh);

			image_transport::SubscriberStatusCallback itssc_depth = std::bind(&Driver::connectCb, this);
			ros::SubscriberStatusCallback rssc_depth = std::bind(&Driver::connectCb, this);
			pub_depth = depth_image_it.advertiseCamera("image", 1, itssc_depth, itssc_depth, rssc_depth, rssc_depth);
		}

		if(depth_aligned_enable_)
		{
			ros::NodeHandle depth_aligned_image_nh(nh, name_ + "/depth_aligned");
			image_transport::ImageTransport depth_aligned_image_it(depth_aligned_image_nh);

			image_transport::SubscriberStatusCallback itssc_depth_aligned = std::bind(&Driver::connectCb, this);
			ros::SubscriberStatusCallback rssc_depth_aligned = std::bind(&Driver::connectCb, this);
			pub_depth_aligned = depth_aligned_image_it.advertiseCamera("image", 1, itssc_depth_aligned, itssc_depth_aligned, rssc_depth_aligned, rssc_depth_aligned);
		}

		if(infrared_enable_ && infrared_left_enable_)
		{
			ros::NodeHandle ir_left_image_nh(nh, name_ + "/infrared_left");
			image_transport::ImageTransport ir_left_image_it(ir_left_image_nh);

			image_transport::SubscriberStatusCallback itssc_ir_left = std::bind(&Driver::connectCb, this);
			ros::SubscriberStatusCallback rssc_ir_left = std::bind(&Driver::connectCb, this);
			pub_ir_left = ir_left_image_it.advertiseCamera("image", 1, itssc_ir_left, itssc_ir_left, rssc_ir_left, rssc_ir_left);
		}

		if(infrared_enable_ && infrared_right_enable_)
		{
			ros::NodeHandle ir_right_image_nh(nh, name_ + "/infrared_right");
			image_transport::ImageTransport ir_right_image_it(ir_right_image_nh);

			image_transport::SubscriberStatusCallback itssc_ir_right = std::bind(&Driver::connectCb, this);
			ros::SubscriberStatusCallback rssc_ir_right = std::bind(&Driver::connectCb, this);
			pub_ir_right = ir_right_image_it.advertiseCamera("image", 1, itssc_ir_right, itssc_ir_right, rssc_ir_right, rssc_ir_right);
		}

		if(imu_enable_)
		{
			ros::NodeHandle imu_nh(nh, name_ +"/imu");
			ros::SubscriberStatusCallback rssc_imu = std::bind(&Driver::connectCb, this);
			pub_imu = imu_nh.advertise<sensor_msgs::Imu>("imu_msg", 1, rssc_imu, rssc_imu);
		}

		device->setDepthFrameCallback(std::bind(&Driver::onDepthFrameCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		device->setDepthAlignedFrameCallback(std::bind(&Driver::onDepthAlignedFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setColorFrameCallback(std::bind(&Driver::onColorFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setIrLeftFrameCallback(std::bind(&Driver::onIrLeftFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setIrRightFrameCallback(std::bind(&Driver::onIrRightFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setIMUFrameCallback(std::bind(&Driver::onIMUFrameCb, this, std::placeholders::_1));

		SCDynServer = std::make_shared<dynamic_reconfigure::Server<structure_core_ros_driver::SCParamsConfig>>(getPrivateNodeHandle());
	    dynamic_reconfigure::Server<structure_core_ros_driver::SCParamsConfig>::CallbackType f;
		f = boost::bind(&Driver::dynParamsCallback, this, _1, _2);
		SCDynServer->setCallback(f);
	}

	void Driver::readParams()
	{
		pnh.param<std::string>("name", name_, "sc");
		pnh.param<bool>("visible_enable", visible_enable_, true);
		pnh.param<bool>("visible_apply_gamma_correction", visible_apply_gamma_correction_, false);
		pnh.param<float>("visible_framerate", visible_framerate_, 30.0);
		pnh.param<std::string>("visible_resolution", visible_resolution_, "Default");
		pnh.param<bool>("infrared_enable", infrared_enable_, false);
		pnh.param<float>("infrared_framerate", infrared_framerate_, 30.0);
		pnh.param<std::string>("infrared_mode", infrared_mode_, "Default");
		pnh.param<std::string>("infrared_resolution", infrared_resolution_, "Default");
		pnh.param<bool>("infrared_disable_intensity_balance", infrared_disable_intensity_balance_, true);
		pnh.param<bool>("imu_enable", imu_enable_, false);
		pnh.param<std::string>("imu_update_rate", imu_update_rate_, "Default");
		pnh.param<bool>("depth_enable", depth_enable_, false);
		pnh.param<bool>("depth_aligned_enable", depth_aligned_enable_, false);
		pnh.param<float>("depth_framerate", depth_framerate_, 30.0);
		pnh.param<std::string>("depth_resolution", depth_resolution_, "Default");
		pnh.param<bool>("depth_apply_correction_before_stream", depth_apply_correction_before_stream_, false);
		pnh.param<std::string>("demosaic_method", demosaic_method_, "EdgeAware");
		pnh.param<float>("initial_projector_power", initial_projector_power_, 1.0);
		pnh.param<bool>("latency_reducer_enabled", true);
		pnh.param<int>("sensor_initialization_timeout", sensor_initialization_timeout_, 6000);
		pnh.param<std::string>("sensor_serial", sensor_serial_, "null");
		pnh.param<bool>("frame_sync_enabled", frame_sync_enabled_, true);

		if(infrared_mode_ == "RightCameraOnly")
			infrared_right_enable_ = true;
		else if(infrared_mode_ == "LeftCameraOnly")
			infrared_left_enable_ = true;
		else
		{
			infrared_right_enable_ = true;
			infrared_left_enable_ = true;
		}

		device->setParams( visible_enable_,
						   visible_apply_gamma_correction_,
						   visible_framerate_,
						   visible_resolution_,
						   infrared_enable_,
						   infrared_framerate_,
						   infrared_mode_,
						   infrared_resolution_,
						   infrared_disable_intensity_balance_,
						   imu_enable_,
						   imu_update_rate_,
						   depth_enable_,
						   depth_aligned_enable_,
						   depth_framerate_,
						   depth_resolution_,
						   depth_apply_correction_before_stream_,
						   demosaic_method_,
						   initial_projector_power_,
						   latency_reducer_enabled_,
						   sensor_initialization_timeout_,
						   sensor_serial_,
						   frame_sync_enabled_,
						   name_);
	}

	void Driver::connectCb()
	{
		NODELET_INFO_STREAM("pub_color subscriber number: " << pub_color.getNumSubscribers());
		NODELET_INFO_STREAM("pub_depth subscriber number:  " << pub_depth.getNumSubscribers());
		NODELET_INFO_STREAM("pub_ir subscriber number:  " << pub_ir_left.getNumSubscribers());
		NODELET_INFO_STREAM("pub_ir subscriber number:  " << pub_ir_right.getNumSubscribers());
		NODELET_INFO_STREAM("pub_imu subscriber number:  " <<  pub_imu.getNumSubscribers());

		if (pub_color.getNumSubscribers() == 0 && pub_depth.getNumSubscribers() == 0 &&
			pub_ir_left.getNumSubscribers() == 0 && pub_ir_right.getNumSubscribers() == 0 &&
			pub_imu.getNumSubscribers() == 0 && pub_depth_aligned.getNumSubscribers() == 0)
		{
				if (sc_streaming)
				{
					sc_streaming = false;
					device->stopStream();
				}
		}
		else if(!sc_streaming)
		{
			sc_streaming = true;
			device->startStream();
		}
	}

	void Driver::dynParamsCallback(structure_core_ros_driver::SCParamsConfig &config, uint32_t level)
	{
			device->setNewParams(config.visible_initial_gain, config.visible_initial_exposure,
								 config.infrared_auto_exposure_enabled, config.infrared_initial_exposure, config.infrared_initial_gain,
								 config.depth_apply_correction, config.depth_range_mode, config.dynamic_calibration_mode);
	}

}
