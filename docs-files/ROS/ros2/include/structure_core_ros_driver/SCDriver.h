/*
    SCDriver.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io

    ROS wrapper for Structure Core. All settings of the Structure Core can be set in sc.launch file.
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_

#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <functional>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/clock.hpp"
#include "rclcpp/time.hpp"
#include "rcl/time.h"

#include <image_transport/image_transport.h>
#include <tf2_ros/transform_broadcaster.h>
#include <structure_core_ros_driver/SCDevice.h>

namespace structure_core
{
	class Driver : public rclcpp::Node
	{
	public:
		Driver(const std::string& node_name = "sc_node",
               const std::string& ros_namespace = "sc",
			   bool intra_process_comms = true) : Node(node_name, ros_namespace, intra_process_comms) 
		{
			onInit();
		};

		virtual ~Driver();

		void onInit();

		void readParams();

		void connectCb();

		void initTfBroadcasters() {
        if (tf_broadcaster_initialized) {
            return;
        }

        //br_color_to_depth_tf =  std::make_shared<tf2_ros::TransformBroadcaster>(); //.reset(new tf2_ros::TransformBroadcaster);
        br_color_to_depth_tf.reset(new tf2_ros::TransformBroadcaster(shared_from_this()));

        tf_broadcaster_initialized = true;
    	}


		inline void onColorFrameCb(std::shared_ptr<sensor_msgs::msg::Image> vis_frame, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
		{
			pub_color.publish(vis_frame, cam_info);
		}

		inline void onDepthFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info, geometry_msgs::msg::TransformStamped color_to_depth_tf)
		{
			pub_depth.publish(depth_frame, cam_info);

			if (!tf_broadcaster_initialized) {
            initTfBroadcasters();
        	}

			br_color_to_depth_tf->sendTransform(color_to_depth_tf);
			//pub_color_to_depth_tf->publish(color_to_depth_tf);
		}

		inline void onDepthAlignedFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
		{
			pub_depth_aligned.publish(depth_frame, cam_info);
		}

		inline void onDepthPointsFrameCb(sensor_msgs::msg::PointCloud2 depth_points_frame)
		{
			pub_depth_points->publish(depth_points_frame);
		}

		inline void onRGBDFrameCb(sensor_msgs::msg::PointCloud2 rgbd_frame)
		{
			pub_rgbd->publish(rgbd_frame);
		}

		inline void onIrLeftFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
		{
			pub_ir_left.publish(ir_frame, cam_info);
		}

		inline void onIrRightFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
		{
			pub_ir_right.publish(ir_frame, cam_info);
		}

		inline void onIMUFrameCb(sensor_msgs::msg::Imu imu_msg)
		{
			pub_imu->publish(imu_msg);
		}

		rcl_interfaces::msg::SetParametersResult onParameterChange(std::vector<rclcpp::Parameter> parameters);

	private:
		std::shared_ptr<structure_core::CameraDevice> device;
		image_transport::CameraPublisher pub_color;
		image_transport::CameraPublisher pub_depth;
		image_transport::CameraPublisher pub_depth_aligned;
		image_transport::CameraPublisher pub_ir_left;
		image_transport::CameraPublisher pub_ir_right;
		rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_rgbd;
		rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_depth_points;
		rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr pub_imu;
		//rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr pub_color_to_depth_tf;
		std::shared_ptr<tf2_ros::TransformBroadcaster>  br_color_to_depth_tf;

		bool tf_broadcaster_initialized = false;
		std::mutex connect_mutex_;

		std::string name_ = "sc";
		bool visible_enable_ = false;
		bool visible_apply_gamma_correction_ = false;
		float visible_framerate_ = 30.0;
		std::string visible_resolution_ = "Default";
		bool infrared_enable_ = false;
		bool infrared_left_enable_ = false;
		bool infrared_right_enable_ = false;
		float infrared_framerate_ = 30.0;
		std::string infrared_mode_ = "Default";
		std::string infrared_resolution_ = "Default";
		bool infrared_disable_intensity_balance_ = true;
		bool imu_enable_ = false;
		std::string imu_update_rate_ = "Default";
		bool depth_enable_ = false;
		bool depth_aligned_enable_ = false;
		bool depth_pcloud_enable_ = false;
		float depth_framerate_ = 30.0;
		std::string depth_resolution_ = "Default";
		bool depth_apply_correction_before_stream_ = false;
		float initial_projector_power_ = 1.0;
		bool latency_reducer_enabled_ = true;
		int sensor_initialization_timeout_ = 6000;
		std::string sensor_serial_ = "null";
		std::string demosaic_method_ = "EdgeAware";
		bool frame_sync_enabled_ = true;
		bool low_latency_imu_ = false;
		bool rgbd_enable_ = false;

		//Dynamic parameteres
		float visible_initial_gain_ = 2.0;
		float visible_initial_exposure_ = 0.016;
		bool infrared_auto_exposure_enabled_ = false;
		float infrared_initial_exposure_ = 0.0146;
		float infrared_initial_gain_ = 3.0;
		bool depth_apply_correction_ = true;
		int depth_range_mode_ = 1;
		int dynamic_calibration_mode_ = 0;

		std::string frame_id;
		bool sc_streaming = false;
	};
}


#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_ */
