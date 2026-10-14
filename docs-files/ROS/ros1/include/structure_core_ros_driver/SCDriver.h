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

#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <tf2_ros/transform_broadcaster.h>
#include <image_transport/image_transport.h>
#include <structure_core_ros_driver/SCDevice.h>

#include <dynamic_reconfigure/server.h>
#include <structure_core_ros_driver/SCParamsConfig.h>

namespace structure_core
{
	class Driver : public nodelet::Nodelet
	{
	public:
		Driver();

		virtual ~Driver();

		virtual void onInit();

		void readParams();

		void connectCb();


		inline void onColorFrameCb(sensor_msgs::ImagePtr vis_frame, sensor_msgs::CameraInfoPtr cam_info)
		{
			pub_color.publish(vis_frame, cam_info);
		}

		inline void onDepthFrameCb(sensor_msgs::ImagePtr depth_frame, sensor_msgs::CameraInfoPtr cam_info, geometry_msgs::TransformStamped color_to_depth_tf)
		{
			pub_depth.publish(depth_frame, cam_info);
			br_color_to_depth.sendTransform(color_to_depth_tf);
		}

		inline void onDepthAlignedFrameCb(sensor_msgs::ImagePtr depth_frame, sensor_msgs::CameraInfoPtr cam_info)
		{
			pub_depth_aligned.publish(depth_frame, cam_info);
		}

		inline void onIrLeftFrameCb(sensor_msgs::ImagePtr ir_frame, sensor_msgs::CameraInfoPtr cam_info)
		{
			pub_ir_left.publish(ir_frame, cam_info);
		}

		inline void onIrRightFrameCb(sensor_msgs::ImagePtr ir_frame, sensor_msgs::CameraInfoPtr cam_info)
		{
			pub_ir_right.publish(ir_frame, cam_info);
		}

		inline void onIMUFrameCb(sensor_msgs::Imu imu_msg)
		{
			pub_imu.publish(imu_msg);
		}

		void dynParamsCallback(structure_core_ros_driver::SCParamsConfig &config, uint32_t level);


	private:
		std::shared_ptr<structure_core::CameraDevice> device;
		ros::NodeHandle  nh;
		ros::NodeHandle pnh;
		image_transport::CameraPublisher pub_color;
		image_transport::CameraPublisher pub_depth;
		image_transport::CameraPublisher pub_depth_aligned;
		image_transport::CameraPublisher pub_ir_left;
		image_transport::CameraPublisher pub_ir_right;
		ros::Publisher pub_imu;
		ros::Publisher pub_color_to_depth_tf;
		tf2_ros::TransformBroadcaster br_color_to_depth;

		std::shared_ptr<dynamic_reconfigure::Server<structure_core_ros_driver::SCParamsConfig>> SCDynServer;

		std::mutex connect_mutex_;

		std::string name_;
		bool visible_enable_ = false;
		float visible_initial_exposure_;
		float visible_initial_gain_;
		bool visible_apply_gamma_correction_;
		float visible_framerate_;
		std::string visible_resolution_;
		bool infrared_enable_ = false;
		bool infrared_left_enable_ = false;
		bool infrared_right_enable_ = false;
		bool infrared_auto_exposure_enabled_;
		float infrared_framerate_;
		std::string infrared_mode_;
		std::string infrared_resolution_;
		float infrared_initial_exposure_;
		int infrared_initial_gain_;
		bool infrared_disable_intensity_balance_;
		bool imu_enable_ = false;
		std::string imu_update_rate_;
		bool depth_enable_ = false;
		bool depth_aligned_enable_ = false;
		float depth_framerate_;
		std::string depth_resolution_;
		std::string depth_range_mode_;
		bool depth_apply_correction_;
		bool depth_apply_correction_before_stream_;
		float initial_projector_power_;
		bool latency_reducer_enabled_;
		int sensor_initialization_timeout_;
		std::string sensor_serial_;
		std::string demosaic_method_;
		bool frame_sync_enabled_;
		bool low_latency_imu_;

		std::string frame_id;
		bool sc_streaming = false;
	};
}

#include <pluginlib/class_list_macros.h>
PLUGINLIB_EXPORT_CLASS(structure_core::Driver, nodelet::Nodelet)

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_ */
