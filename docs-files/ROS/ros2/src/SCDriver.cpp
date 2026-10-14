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
#include <string>
#include <iostream>
#include "structure_core_ros_driver/SCDriver.h"

namespace structure_core
{

	Driver::~Driver() {}

	void Driver::onInit()
	{

		device = std::make_shared<structure_core::CameraDevice>();

		readParams();

		// Dynamic parameters callback
		register_param_change_callback(std::bind(&Driver::onParameterChange, this, std::placeholders::_1));

		std::lock_guard<std::mutex> lock(connect_mutex_);

		//initTfBroadcasters();

		if(visible_enable_)
		{
			pub_color = image_transport::create_camera_publisher(this, "rgb/image");
		}

		if(depth_enable_)
		{
			pub_depth = image_transport::create_camera_publisher(this, "depth/image");
			//pub_color_to_depth_tf = create_publisher<std_msgs::msg::Float32MultiArray>("depth/color_to_depth_tf", 1);

			if(depth_pcloud_enable_)
				pub_depth_points = create_publisher<sensor_msgs::msg::PointCloud2>("depth/points", 1);
		}

		if(depth_aligned_enable_)
		{
			pub_depth_aligned = image_transport::create_camera_publisher(this, "depth_aligned/image");
		}

		if(rgbd_enable_)
		{
			pub_rgbd = create_publisher<sensor_msgs::msg::PointCloud2>("rgbd/points", 1);
		}

		if(infrared_enable_ && infrared_left_enable_)
		{
			pub_ir_left = image_transport::create_camera_publisher(this, "depth/infrared_left");
		}

		if(infrared_enable_ && infrared_right_enable_)
		{
			pub_ir_right = image_transport::create_camera_publisher(this, "depth/infrared_right");
		}

		if(imu_enable_)
		{
			pub_imu = create_publisher<sensor_msgs::msg::Imu>("imu/imu_msg", 1);
		}

		device->setDepthFrameCallback(std::bind(&Driver::onDepthFrameCb, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		device->setDepthAlignedFrameCallback(std::bind(&Driver::onDepthAlignedFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setColorFrameCallback(std::bind(&Driver::onColorFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setDepthPointsCallback(std::bind(&Driver::onDepthPointsFrameCb, this, std::placeholders::_1));
		device->setRGBDCallback(std::bind(&Driver::onRGBDFrameCb, this, std::placeholders::_1));
		device->setIrLeftFrameCallback(std::bind(&Driver::onIrLeftFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setIrRightFrameCallback(std::bind(&Driver::onIrRightFrameCb, this, std::placeholders::_1, std::placeholders::_2));
		device->setIMUFrameCallback(std::bind(&Driver::onIMUFrameCb, this, std::placeholders::_1));

		connectCb();

	}

	void Driver::readParams()
	{
		// Static parameters
		get_parameter_or("visible_enable", visible_enable_, visible_enable_);
		get_parameter_or("visible_apply_gamma_correction", visible_apply_gamma_correction_, visible_apply_gamma_correction_);
		get_parameter_or("visible_framerate", visible_framerate_, visible_framerate_);
		get_parameter_or("visible_resolution", visible_resolution_, visible_resolution_);
		get_parameter_or("infrared_enable", infrared_enable_, infrared_enable_);
		get_parameter_or("infrared_framerate", infrared_framerate_, infrared_framerate_);
		get_parameter_or("infrared_mode", infrared_mode_, infrared_mode_);
		get_parameter_or("infrared_resolution", infrared_resolution_, infrared_resolution_);
		get_parameter_or("infrared_disable_intensity_balance", infrared_disable_intensity_balance_, infrared_disable_intensity_balance_);
		get_parameter_or("imu_enable", imu_enable_, imu_enable_);
		get_parameter_or("imu_update_rate", imu_update_rate_, imu_update_rate_);
		get_parameter_or("depth_enable", depth_enable_, depth_enable_);
		get_parameter_or("depth_aligned_enable", depth_aligned_enable_, depth_aligned_enable_);
		get_parameter_or("depth_pcloud_enable", depth_pcloud_enable_, depth_pcloud_enable_);
		get_parameter_or("depth_framerate", depth_framerate_, depth_framerate_);
		get_parameter_or("depth_resolution", depth_resolution_, depth_resolution_);
		get_parameter_or("depth_apply_correction_before_stream", depth_apply_correction_before_stream_, depth_apply_correction_before_stream_);
		get_parameter_or("demosaic_method", demosaic_method_, demosaic_method_);
		get_parameter_or("initial_projector_power", initial_projector_power_, initial_projector_power_);
		get_parameter_or("latency_reducer_enabled", latency_reducer_enabled_, latency_reducer_enabled_);
		get_parameter_or("sensor_initialization_timeout", sensor_initialization_timeout_, sensor_initialization_timeout_);
		get_parameter_or("sensor_serial", sensor_serial_, sensor_serial_);
		get_parameter_or("frame_sync_enabled", frame_sync_enabled_, frame_sync_enabled_);
		get_parameter_or("rgbd_enable", rgbd_enable_, rgbd_enable_);
		get_parameter_or("name", name_, name_);

		if(infrared_mode_ == "RightCameraOnly")
			infrared_right_enable_ = true;
		else if(infrared_mode_ == "LeftCameraOnly")
			infrared_left_enable_ = true;
		else
		{
			infrared_right_enable_ = true;
			infrared_left_enable_ = true;
		}

		if(rgbd_enable_)
		{
			visible_enable_ = true;
			depth_enable_ = true;
		}

		//Dynamic parameteres
		get_parameter_or("visible_initial_gain", visible_initial_gain_, visible_initial_gain_);
		get_parameter_or("visible_initial_exposure", visible_initial_exposure_, visible_initial_exposure_);
		get_parameter_or("infrared_auto_exposure_enabled", infrared_auto_exposure_enabled_, infrared_auto_exposure_enabled_);
		get_parameter_or("infrared_initial_exposure", infrared_initial_exposure_, infrared_initial_exposure_);
		get_parameter_or("infrared_initial_gain", infrared_initial_gain_, infrared_initial_gain_);
		get_parameter_or("depth_apply_correction", depth_apply_correction_, depth_apply_correction_);
		get_parameter_or("depth_range_mode", depth_range_mode_, depth_range_mode_);
		get_parameter_or("dynamic_calibration_mode", dynamic_calibration_mode_, dynamic_calibration_mode_);

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
						   depth_pcloud_enable_,
						   depth_framerate_,
						   depth_resolution_,
						   depth_apply_correction_before_stream_,
						   demosaic_method_,
						   initial_projector_power_,
						   latency_reducer_enabled_,
						   sensor_initialization_timeout_,
						   sensor_serial_,
						   frame_sync_enabled_,
						   rgbd_enable_,
						   visible_initial_gain_,
						   visible_initial_exposure_,
						   infrared_auto_exposure_enabled_,
						   infrared_initial_exposure_,
						   infrared_initial_gain_,
						   depth_apply_correction_,
						   depth_range_mode_,
						   dynamic_calibration_mode_,
						   name_);
	}


	rcl_interfaces::msg::SetParametersResult Driver::onParameterChange(std::vector<rclcpp::Parameter> parameters) 
	{
        auto result = rcl_interfaces::msg::SetParametersResult();
        result.successful = false;

        for (size_t i = 0; i < parameters.size(); i++) {
            rclcpp::Parameter param = parameters[i];

            std::cout << param.get_name() << param  << std::endl;

            if (param.get_name() == "visible_initial_gain") 
            {
                if (param.get_type() == rclcpp::PARAMETER_DOUBLE &&
                    param.as_double() >= 1.0 && param.as_double() <= 8.0) 
                {
                	visible_initial_gain_ = param.as_double();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide FLOATING POINT value in the range [1.0, 8.0] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "visible_initial_exposure") 
            {
                if (param.get_type() == rclcpp::PARAMETER_DOUBLE &&
                	param.as_double() >= 0.0 && param.as_double() <= 0.03) 
                {
                	visible_initial_exposure_ = param.as_double();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide FLOATING POINT value in the range [0.0, 0.03] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "infrared_auto_exposure_enabled") 
            {
                if (param.get_type() == rclcpp::PARAMETER_BOOL) 
                {
                	infrared_auto_exposure_enabled_ = param.as_bool();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide BOOL value for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "infrared_initial_exposure") 
            {
                if (param.get_type() == rclcpp::PARAMETER_DOUBLE &&
                	param.as_double() >= 0.0 && param.as_double() <= 0.03) 
                {
                	infrared_initial_exposure_ = param.as_double();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide FLOATING POINT value in the range [0.0, 0.03] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "infrared_initial_gain") 
            {
                if (param.get_type() == rclcpp::PARAMETER_DOUBLE &&
                	param.as_double() >= 1.0 && param.as_double() <= 3.0) 
                {
                	infrared_initial_gain_ = param.as_double();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide FLOATING POINT value in the range [0.0, 3.0] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "depth_apply_correction") 
            {
                if (param.get_type() == rclcpp::PARAMETER_BOOL) 
                {
                	depth_apply_correction_ = param.as_bool();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide BOOL value for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "depth_range_mode") 
            {
                if (param.get_type() == rclcpp::PARAMETER_INTEGER &&
                	param.as_int() >= -1 && param.as_int() <= 5) 
                {
                	depth_range_mode_ = param.as_int();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide  INTEGER value in the range [-1, 5] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }
            else if (param.get_name() == "dynamic_calibration_mode") 
            {
                if (param.get_type() == rclcpp::PARAMETER_INTEGER &&
                	param.as_int() >= 0 && param.as_int() <= 2) 
                {
                	dynamic_calibration_mode_ = param.as_int();
                }
                else 
                {
                    RCLCPP_WARN(get_logger(), "Provide  INTEGER value in the range [0, 2] for '%s'", param.get_name().c_str());
                    result.successful = false;
                    return result;
				}
            }

    	}

        auto dyn_params_set = device->setNewDynParams(visible_initial_gain_, visible_initial_exposure_, infrared_auto_exposure_enabled_,
        	 				                          infrared_initial_exposure_, infrared_initial_gain_,
						                              depth_apply_correction_, depth_range_mode_, dynamic_calibration_mode_);
 
        if(dyn_params_set)
        	result.successful = true;
        else
        	result.successful = false;

        return result;
	}

	void Driver::connectCb()
	{

		sc_streaming = true;
		device->startStream();
	}


}

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
//RCLCPP_COMPONENTS_REGISTER_NODE(composition::Driver)

#include "class_loader/register_macro.hpp"

// Register the component with class_loader.
// This acts as a sort of entry point, allowing the component to be discoverable when its library
// is being loaded into a running process.
CLASS_LOADER_REGISTER_CLASS(structure_core::Driver, rclcpp::Node)