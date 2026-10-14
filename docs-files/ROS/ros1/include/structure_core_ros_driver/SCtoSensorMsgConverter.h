/*
    STtoSensorMsgConverter.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_

#include <sensor_msgs/Image.h>
#include <sensor_msgs/Imu.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <sensor_msgs/image_encodings.h>
#include <sensor_msgs/distortion_models.h>
#include <std_msgs/Float32MultiArray.h>
#include <std_msgs/MultiArrayDimension.h>
#include <geometry_msgs/TransformStamped.h>

#include <boost/make_shared.hpp>
#include <iterator>
#include <functional>

#include <iostream>
#include "SCDevice.h"

typedef std::function<void(sensor_msgs::ImagePtr image, sensor_msgs::CameraInfoPtr cam_info)> ConverterCallbackFunction;
typedef std::function<void(sensor_msgs::ImagePtr image,  sensor_msgs::CameraInfoPtr cam_info, geometry_msgs::TransformStamped color_to_depth_tf)> DepthConverterCallbackFunction;
typedef std::function<void(sensor_msgs::ImagePtr image,  sensor_msgs::CameraInfoPtr cam_info)> DepthAlignedConverterCallbackFunction;
typedef std::function<void(sensor_msgs::Imu imu_msg)> ConverterIMUCallbackFunction;
typedef sensor_msgs::PointCloud2 PointCloud;

namespace structure_core
{
	class Converter
	{
		ConverterCallbackFunction callback_;
		ConverterCallbackFunction color_callback;
		DepthConverterCallbackFunction depth_callback;
		DepthAlignedConverterCallbackFunction depth_aligned_callback;
		ConverterCallbackFunction ir_left_callback;
		ConverterCallbackFunction ir_right_callback;
		ConverterIMUCallbackFunction imu_callback;

		std::string name;
		ros::Time base_time;
		bool base_time_set = false;
		double const g2ms2 = -9.81;


    public:
		Converter()
    	{
			//callback_(0);
    	}

		void setCallback(ConverterCallbackFunction& callback)
		{
			callback_ = callback;
		}

		void setColorCallback(ConverterCallbackFunction& callback)
		{
			color_callback = callback;
		}

		void setDepthCallback(DepthConverterCallbackFunction& callback)
		{
			depth_callback = callback;
		}

		void setDepthAlignedCallback(DepthAlignedConverterCallbackFunction& callback)
		{
			depth_aligned_callback = callback;
		}

		void setIrLeftCallback(ConverterCallbackFunction& callback)
		{
			ir_left_callback = callback;
		}

		void setIrRightCallback(ConverterCallbackFunction& callback)
		{
			ir_right_callback = callback;
		}

		void setIMUCallback(ConverterIMUCallbackFunction& callback)
		{
			imu_callback = callback;
		};

		bool isMono(const ST::ColorFrame &visFrame)
		{
			return visFrame.width() * visFrame.height() == visFrame.rgbSize();
		}

		std::string getEncoding(const ST::ColorFrame &visFrame)
		{
			return isMono(visFrame) ? sensor_msgs::image_encodings::MONO8 : sensor_msgs::image_encodings::RGB8;
		}

		int getStep(const ST::ColorFrame &visFrame)
		{
			return isMono(visFrame) ? 1 : 3;
		}

		void setNamespace(std::string camera_name)
		{
			name = camera_name;
		}

		template <typename M>
		ros::Time getSCtoROSTime(M&& message)
		{
			if (!base_time_set)
			{
				base_time = ros::Time::now() - ros::Duration(ST::getTimestampNow());
				base_time_set = true;
			}

			return base_time + ros::Duration(message.timestamp());
		}

		void onColorFrameAvailableEvent(const ST::ColorFrame &visFrame)
		{
			sensor_msgs::ImagePtr image = boost::make_shared<sensor_msgs::Image>();

			std::string frame_name = name + "_color_frame";
			ros::Time device_time = getSCtoROSTime(visFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = visFrame.width();
			image->height = visFrame.height();
			image->encoding = getEncoding(visFrame);
			image->step = sizeof(unsigned char) * getStep(visFrame) * image->width;
			std::size_t data_size = visFrame.rgbSize();
			image->data.resize(data_size);
			memcpy(&image->data[0], visFrame.rgbData(), data_size);

			sensor_msgs::CameraInfoPtr cam_info = populateCamInfo(visFrame.intrinsics(), device_time, frame_name);

			color_callback(image, cam_info);
		}

		void onDepthFrameAvailableEvent(const ST::DepthFrame &depthFrame)
		{
			sensor_msgs::ImagePtr image = boost::make_shared<sensor_msgs::Image>();

			std::string frame_name = name + "_depth_frame";
			ros::Time device_time =  getSCtoROSTime(depthFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = depthFrame.width();
			image->height = depthFrame.height();
			image->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
			image->step = sizeof(unsigned char) * 4 * image->width;
			std::size_t data_size = image->step * image->height;
			image->data.resize(data_size);
			memcpy(&image->data[0], depthFrame.depthInMeters(), data_size);

			sensor_msgs::CameraInfoPtr cam_info = populateCamInfo(depthFrame.intrinsics(), device_time, frame_name);

			ST::Matrix4 color_in_depth_frame = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
			geometry_msgs::TransformStamped color_to_depth_tf = createColorToDepthTf(color_in_depth_frame, device_time);

			depth_callback(image, cam_info, color_to_depth_tf);
		};

		void onDepthAlignedAvailableEvent(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
		{
			sensor_msgs::ImagePtr image = boost::make_shared<sensor_msgs::Image>();

			std::string frame_name = name + "_depth_aligned_frame";
			ros::Time device_time =  getSCtoROSTime(depthFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = visFrame.width();
			image->height = visFrame.height();
			image->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
			image->step = sizeof(unsigned char) * 4 * image->width;
			std::size_t data_size = image->step * image->height;
			image->data.resize(data_size);

			const auto visible_from_depth = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
			ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();
			assert (depth_intrinsics.k1k2k3p1p2AreZero());

			const ST::ColorFrame undistorted_color_frame = visFrame.undistorted();
			ST::Intrinsics color_intrinsics = undistorted_color_frame.intrinsics();

			float* registered_data = reinterpret_cast<float*>(&image->data[0]);

			const float* buf_depth = depthFrame.depthInMillimeters();
			double to_meters_multiplier = 0.001;

			float depth_frame_meters[depthFrame.height() * depthFrame.width()];

			for(int y = 0; y < depthFrame.height(); y++)
			{
				for(int x = 0; x < depthFrame.width(); x++ )
				{

					std::size_t pixel_offset = (y*depthFrame.width()) + x;
					auto depth_in_meters = buf_depth[pixel_offset] * to_meters_multiplier;

					if(!std::isnan(depth_in_meters))
					{
						int depth_point[2] = {x, y};
						int visible_point[2] = {0};
						double depth_3d_point[3] = {0};

						alignDepthPointToVisFrame(depth_point, visible_point, depth_3d_point, depth_in_meters, 
												  depth_intrinsics, color_intrinsics, visible_from_depth);

						int pixel_color_offset = (visible_point[1]*undistorted_color_frame.width()) + visible_point[0];

						if (pixel_color_offset > 0 and pixel_color_offset <  depthFrame.height() * depthFrame.width())
						{
							float& reg_depth = registered_data[pixel_color_offset];
							reg_depth = depth_in_meters;
						}
					}
				}
			}

			sensor_msgs::CameraInfoPtr cam_info = populateCamInfo(depthFrame.intrinsics(), device_time, frame_name);

			depth_aligned_callback(image, cam_info);
		};

		void onIROneFrameAvailableEvent(const ST::InfraredFrame &irFrame, bool ir_left_image)
		{
			sensor_msgs::ImagePtr image = boost::make_shared<sensor_msgs::Image>();

			std::string frame_name = name + "_ir_frame";
			ros::Time device_time = getSCtoROSTime(irFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = irFrame.width();
			image->height = irFrame.height();
			image->encoding = sensor_msgs::image_encodings::TYPE_16UC1;
			image->step = sizeof(unsigned char) * 2 * image->width;
			std::size_t data_size = image->step * image->height;
			image->data.resize(data_size);

			memcpy(&image->data[0], irFrame.data(), data_size);

			sensor_msgs::CameraInfoPtr cam_info = populateCamInfo(irFrame.intrinsics(), device_time, frame_name);

			if(ir_left_image)
				ir_left_callback(image, cam_info);
			else
				ir_right_callback(image, cam_info);
		};

		void onIRBothFramesAvailableEvent(const ST::InfraredFrame &irFrame)
		{
			sensor_msgs::ImagePtr left_image = boost::make_shared<sensor_msgs::Image>();
			sensor_msgs::ImagePtr right_image = boost::make_shared<sensor_msgs::Image>();

			std::string frame_name = name + "_ir_frame";
			ros::Time device_time = getSCtoROSTime(irFrame);
			left_image->header.frame_id = frame_name;
			right_image->header.frame_id = frame_name;
			left_image->header.stamp = device_time;
			right_image->header.stamp = device_time;

			left_image->width = irFrame.width() / 2;
			right_image->width = irFrame.width() / 2;
			left_image->height = irFrame.height();
			right_image->height = irFrame.height();

			left_image->encoding = sensor_msgs::image_encodings::MONO8;
			right_image->encoding = sensor_msgs::image_encodings::MONO8;

			left_image->step = sizeof(unsigned char) * 1 * left_image->width;
			right_image->step = sizeof(unsigned char) * 1 * right_image->width;

			std::size_t data_size = left_image->step * left_image->height;

			const uint16_t* buf_ir = irFrame.data();

			for(int v = 0; v < irFrame.height(); v++)
			{
				std::size_t pixelLocation = v * irFrame.width();

				right_image->data.insert(right_image->data.end(), &buf_ir[pixelLocation], &buf_ir[pixelLocation + irFrame.width()/2]);
				left_image->data.insert(left_image->data.end(), &buf_ir[pixelLocation + irFrame.width()/2], &buf_ir[pixelLocation + irFrame.width()]);
			}

			sensor_msgs::CameraInfoPtr cam_info = populateCamInfo(irFrame.intrinsics(), device_time, frame_name);

			ir_left_callback(left_image, cam_info);
			ir_right_callback(right_image, cam_info);
		};

		void onIMUAvailableEvent(const ST::AccelerometerEvent &accelEvent, const ST::GyroscopeEvent& gyroEvent)
		{
			sensor_msgs::Imu imu_msg;

			imu_msg.header.frame_id = name + "_imu_frame";
			ros::Time device_time = getSCtoROSTime(accelEvent);
			imu_msg.header.stamp = device_time;
			imu_msg.linear_acceleration.x = -accelEvent.acceleration().z * g2ms2;
			imu_msg.linear_acceleration.y = accelEvent.acceleration().y * g2ms2;
			imu_msg.linear_acceleration.z = accelEvent.acceleration().x * g2ms2;

			imu_msg.angular_velocity.x = -gyroEvent.rotationRate().z;
			imu_msg.angular_velocity.y = gyroEvent.rotationRate().y;
			imu_msg.angular_velocity.z = gyroEvent.rotationRate().x;

			imu_callback(imu_msg);
		}

		void alignDepthPointToVisFrame(const int *depth_point, int *visible_point, double *depth_3d_point, const double depth_in_meters,
		 							   const ST::Intrinsics &depth_intrinsics, const ST::Intrinsics &color_intrinsics, const ST::Matrix4 &depth_color_transform)
		{
			depth_3d_point[0] = depth_in_meters * (depth_point[0] - depth_intrinsics.cx) / depth_intrinsics.fx;
			depth_3d_point[1] = depth_in_meters * (depth_point[1] - depth_intrinsics.cy) / depth_intrinsics.fy;
			depth_3d_point[2] = depth_in_meters;

			ST::Vector3f xyz_depth(depth_3d_point[0], depth_3d_point[1], depth_3d_point[2]);

			auto xyz_visible = depth_color_transform * xyz_depth;

			float float_visible_col = (xyz_visible.x * color_intrinsics.fx /xyz_visible.z) + color_intrinsics.cx;
			float float_visible_row = (xyz_visible.y * color_intrinsics.fy /xyz_visible.z) + color_intrinsics.cy;

			visible_point[0] = std::round(float_visible_col);
			visible_point[1] = std::round(float_visible_row);
		}

		geometry_msgs::TransformStamped createColorToDepthTf(ST::Matrix4 color_in_depth_frame, ros::Time device_time)
		{
			ST::Vector4 rotQ = color_in_depth_frame.rotationAsQuaternion();
		    ST::Vector3f translation = color_in_depth_frame.translation();

		    geometry_msgs::TransformStamped color_to_depth_tf;

		    color_to_depth_tf.header.stamp = device_time;
		    color_to_depth_tf.header.frame_id = name + "_depth_frame";
		    color_to_depth_tf.child_frame_id = name + "_color_frame";

		    color_to_depth_tf.transform.translation.x = translation.x;
		    color_to_depth_tf.transform.translation.y = translation.y;
		    color_to_depth_tf.transform.translation.z = translation.z;

		    color_to_depth_tf.transform.rotation.x = rotQ.x;
		    color_to_depth_tf.transform.rotation.y = rotQ.y;
		    color_to_depth_tf.transform.rotation.z = rotQ.z;
		    color_to_depth_tf.transform.rotation.w = rotQ.w;

		    return color_to_depth_tf;
		}

		sensor_msgs::CameraInfoPtr populateCamInfo(const ST::Intrinsics &intrinics, ros::Time device_time, std::string frame_name)
		{
			sensor_msgs::CameraInfoPtr cam_info = boost::make_shared<sensor_msgs::CameraInfo>();

			cam_info->header.stamp = device_time;
			cam_info->header.frame_id = frame_name;
			cam_info->height = intrinics.height;
			cam_info->width = intrinics.width;

			// No distortion
			cam_info->D.resize(5, 0);
			cam_info->distortion_model = sensor_msgs::distortion_models::PLUMB_BOB;

			cam_info->K[0] = intrinics.fx;
			cam_info->K[1] = 0;
			cam_info->K[2] = intrinics.cx;
			cam_info->K[3] = 0;
			cam_info->K[4] = intrinics.fy;
			cam_info->K[5] = intrinics.cy;
			cam_info->K[6] = 0;
			cam_info->K[7] = 0;
			cam_info->K[8] = 1;

			// No separate rectified image plane, so R = I
			cam_info->R.assign(0);
			cam_info->R[0] = cam_info->R[4] = cam_info->R[8] = 1;

			cam_info->P[0] = intrinics.fx;
			cam_info->P[1] = 0;
			cam_info->P[2] = intrinics.cx;
			cam_info->P[3] = 0;
			cam_info->P[4] = 0;
			cam_info->P[5] = intrinics.fy;
			cam_info->P[6] = intrinics.cy;
			cam_info->P[7] = 0;
			cam_info->P[8] = 0;
			cam_info->P[9] = 0;
			cam_info->P[10] = 1;
			cam_info->P[11] = 0;

			return cam_info;
		}
	};
}

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_ */
