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

#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/distortion_models.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <std_msgs/msg/multi_array_dimension.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.h>

#include <iterator>
#include <functional>

#include <iostream>
#include "SCDevice.h"

typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> ConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image,  std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info, geometry_msgs::msg::TransformStamped color_to_depth_tf)> DepthConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image,  std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> DepthAlignedConverterCallbackFunction;
typedef std::function<void(sensor_msgs::msg::Imu imu_msg)> ConverterIMUCallbackFunction;
typedef std::function<void(sensor_msgs::msg::PointCloud2 rgbd_points)> ConverterDepthPointsCallbackFunction;
typedef std::function<void(sensor_msgs::msg::PointCloud2 rgbd_points)> ConverterRGBDCallbackFunction;

namespace structure_core
{
	class Converter
	{
		ConverterCallbackFunction callback_;
		ConverterCallbackFunction color_callback;
		DepthConverterCallbackFunction depth_callback;
		DepthAlignedConverterCallbackFunction depth_aligned_callback;
		ConverterDepthPointsCallbackFunction depth_points_callback;
		ConverterCallbackFunction ir_left_callback;
		ConverterCallbackFunction ir_right_callback;
		ConverterIMUCallbackFunction imu_callback;
		ConverterRGBDCallbackFunction rgbd_callback;

		rclcpp::Clock ros_clock;
		rclcpp::Time device_time;

		std::string name;

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

    	void setDepthPointsCallback(ConverterDepthPointsCallbackFunction& callback)
		{
			depth_points_callback = callback;
		}

		void setRGBDCallback(ConverterRGBDCallbackFunction& callback)
		{
			rgbd_callback = callback;
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
		rclcpp::Time getSCtoROSTime(M&& message)
		{
			return ros_clock.now() + rclcpp::Duration(message.timestamp() - ST::getTimestampNow());
		}


		void onColorFrameAvailableEvent(const ST::ColorFrame &visFrame)
		{
			std::shared_ptr<sensor_msgs::msg::Image> image = std::make_shared<sensor_msgs::msg::Image>();

			std::string frame_name = name + "_color_frame";
			device_time = getSCtoROSTime(visFrame);
			image->header.stamp = device_time;			
			image->header.frame_id = frame_name;
			image->width = visFrame.width();
			image->height = visFrame.height();
			image->encoding = getEncoding(visFrame);
			image->step = sizeof(unsigned char) * getStep(visFrame) * image->width;
			std::size_t data_size = visFrame.rgbSize();
			image->data.resize(data_size);
			memcpy(&image->data[0], visFrame.rgbData(), data_size);

			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = populateCamInfo(visFrame.intrinsics(), device_time, frame_name);

			color_callback(image, cam_info);
		}

		void onDepthFrameAvailableEvent(const ST::DepthFrame &depthFrame)
		{
			std::shared_ptr<sensor_msgs::msg::Image> image = std::make_shared<sensor_msgs::msg::Image>();

			std::string frame_name = name + "_depth_frame";
			device_time = getSCtoROSTime(depthFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = depthFrame.width();
			image->height = depthFrame.height();
			image->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
			image->step = sizeof(unsigned char) * 4 * image->width;
			std::size_t data_size = image->step * image->height;
			image->data.resize(data_size);

			const float* buf = depthFrame.depthInMillimeters();

			float depth_frame_meters[depthFrame.height() * depthFrame.width()];
			float to_meters_multiplier = 0.001f;

			for(int y = 0; y < depthFrame.height(); y++)
			{
				for(int x = 0; x < depthFrame.width(); x++)
				{
					std::size_t pixel_offset = (y*depthFrame.width()) + x;
					depth_frame_meters[pixel_offset] = buf[pixel_offset] * to_meters_multiplier;
				}
			}

			memcpy(&image->data[0], depth_frame_meters, data_size);

			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = populateCamInfo(depthFrame.intrinsics(), device_time, frame_name);

			ST::Matrix4 color_in_depth_frame = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
			geometry_msgs::msg::TransformStamped color_to_depth_tf = createColorToDepthTf(color_in_depth_frame, device_time);

			depth_callback(image, cam_info, color_to_depth_tf);
		};

		void onDepthAlignedAvailableEvent(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
		{
			std::shared_ptr<sensor_msgs::msg::Image> image = std::make_shared<sensor_msgs::msg::Image>();

			std::string frame_name = name + "_depth_aligned_frame";
			device_time =  getSCtoROSTime(depthFrame);
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

			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = populateCamInfo(depthFrame.intrinsics(), device_time, frame_name);

			depth_aligned_callback(image, cam_info);
		};

		void onIROneFrameAvailableEvent(const ST::InfraredFrame &irFrame, bool ir_left_image)
		{
			std::shared_ptr<sensor_msgs::msg::Image> image = std::make_shared<sensor_msgs::msg::Image>();

			std::string frame_name = name + "_ir_frame";
			device_time = getSCtoROSTime(irFrame);
			image->header.stamp = device_time;
			image->header.frame_id = frame_name;
			image->width = irFrame.width();
			image->height = irFrame.height();
			image->encoding = sensor_msgs::image_encodings::TYPE_16UC1;
			image->step = sizeof(unsigned char) * 2 * image->width;
			std::size_t data_size = image->step * image->height;
			image->data.resize(data_size);

			memcpy(&image->data[0], irFrame.data(), data_size);

			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = populateCamInfo(irFrame.intrinsics(), device_time, frame_name);

			if(ir_left_image)
				ir_left_callback(image, cam_info);
			else
				ir_right_callback(image, cam_info);
		};

		void onIRBothFramesAvailableEvent(const ST::InfraredFrame &irFrame)
		{
			std::shared_ptr<sensor_msgs::msg::Image> left_image = std::make_shared<sensor_msgs::msg::Image>();
			std::shared_ptr<sensor_msgs::msg::Image> right_image = std::make_shared<sensor_msgs::msg::Image>();

			std::string frame_name = name + "_ir_frame";
			device_time = getSCtoROSTime(irFrame);
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
				for(int u = 0; u < irFrame.width(); u++)
				{
					std::size_t pixelOffset = (v*irFrame.width()) + u;
					uint16_t ir_pixel = buf_ir[pixelOffset] >> 2 ;

					if(u < right_image->width)
					{
						right_image->data.push_back(ir_pixel);
					}
					else
					{
						left_image->data.push_back(ir_pixel);
					}
				}
			}
			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = populateCamInfo(irFrame.intrinsics(), device_time, frame_name);

			ir_left_callback(left_image, cam_info);
			ir_right_callback(right_image, cam_info);
		};

		void onIMUAvailableEvent(const ST::AccelerometerEvent &accelEvent, const ST::GyroscopeEvent& gyroEvent)
		{
			sensor_msgs::msg::Imu imu_msg;

			device_time = getSCtoROSTime(accelEvent);
			imu_msg.header.stamp = device_time;
			imu_msg.linear_acceleration.x = accelEvent.acceleration().x;
			imu_msg.linear_acceleration.y = accelEvent.acceleration().y;
			imu_msg.linear_acceleration.z = accelEvent.acceleration().z;

			imu_msg.angular_velocity.x = gyroEvent.rotationRate().x;
			imu_msg.angular_velocity.y = gyroEvent.rotationRate().y;
			imu_msg.angular_velocity.z = gyroEvent.rotationRate().z;

			imu_callback(imu_msg);
		}

		void onRGBDAvailableEvent(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
		{
			const float* buf_depth = depthFrame.depthInMillimeters();

			const auto visible_from_depth = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
			ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();
			assert (depth_intrinsics.k1k2k3p1p2AreZero());

			const ST::ColorFrame undistorted_color_frame = visFrame.undistorted();
			ST::Intrinsics color_intrinsics = undistorted_color_frame.intrinsics();
			const uint8_t* buf_color = undistorted_color_frame.rgbData();

			int width = 0;

			//find number non NAN points
			for(int v = 0; v < depthFrame.height(); v++)
			{
				for(int u = 0; u < depthFrame.width(); u++)
				{
					std::size_t pixel_offset = (v*depthFrame.width()) + u;
					if(!std::isnan(buf_depth[pixel_offset])) width++;
				}
			}

			std::shared_ptr<sensor_msgs::msg::PointCloud2> cloud_msg = std::make_shared<sensor_msgs::msg::PointCloud2>();

			cloud_msg->header.frame_id = name + "_cloud_tf_frame";
			device_time = getSCtoROSTime(depthFrame);
			cloud_msg->header.stamp = device_time;
			cloud_msg->height = 1;
			cloud_msg->width  = width;
			cloud_msg->is_dense = false;
			cloud_msg->is_bigendian = false;

			sensor_msgs::PointCloud2Modifier pcd_modifier(*cloud_msg);
			pcd_modifier.setPointCloud2FieldsByString(2, "xyz", "rgb");

			sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
			sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
			sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");
			sensor_msgs::PointCloud2Iterator<uint8_t> iter_r(*cloud_msg, "r");
			sensor_msgs::PointCloud2Iterator<uint8_t> iter_g(*cloud_msg, "g");
			sensor_msgs::PointCloud2Iterator<uint8_t> iter_b(*cloud_msg, "b");
			sensor_msgs::PointCloud2Iterator<uint8_t> iter_a(*cloud_msg, "a");

			float bad_point = std::numeric_limits<float>::quiet_NaN ();

			double to_meters_multiplier = 0.001;

			for(int v = 0; v < depthFrame.height(); v++)
			{
				for(int u = 0; u < depthFrame.width(); u++)
				{
					std::size_t pixel_offset = (v*depthFrame.width()) + u;
					auto depth_in_meters = buf_depth[pixel_offset] * to_meters_multiplier;

					if(!std::isnan(depth_in_meters))
					{

						int depth_point[2] = {u, v};
						int visible_point[2] = {0};
						double depth_3d_point[3] = {0};

						alignDepthPointToVisFrame(depth_point, visible_point, depth_3d_point, depth_in_meters, 
												  depth_intrinsics, color_intrinsics, visible_from_depth);

						if(isMono(visFrame))
						{
							std::size_t pixel_color_offset = (visible_point[1]*undistorted_color_frame.width()) + visible_point[0];

							*iter_a = 255;
							*iter_r = buf_color[pixel_color_offset];
							*iter_g = buf_color[pixel_color_offset];
							*iter_b = buf_color[pixel_color_offset];
						}
						else
						{
							std::size_t pixel_color_offset = (visible_point[1]*undistorted_color_frame.width() * 3) + visible_point[0] * 3;

							*iter_a = 255;
							*iter_r = buf_color[pixel_color_offset];
							*iter_g = buf_color[pixel_color_offset+1];
							*iter_b = buf_color[pixel_color_offset+2];
						}

						*iter_x = depth_3d_point[2];
						*iter_y = -depth_3d_point[0];
						*iter_z = -depth_3d_point[1];

						++iter_x, ++iter_y, ++iter_z, ++iter_a, ++iter_r, ++iter_g, ++iter_b;
					}			
				}
			}
			rgbd_callback(*cloud_msg);
		}

		void onDepthPointsAvailableEvent(const ST::DepthFrame &depthFrame)
		{

			const float* buf_depth = depthFrame.depthInMillimeters();

			const auto visible_from_depth = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
			ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();
			assert (depth_intrinsics.k1k2k3p1p2AreZero());

			float bad_point = std::numeric_limits<float>::quiet_NaN ();

			double to_meters_multiplier = 0.001;
			int width = 0;

			//find number non NAN points
			for(int v = 0; v < depthFrame.height(); v++)
			{
				for(int u = 0; u < depthFrame.width(); u++)
				{
					std::size_t pixel_offset = (v*depthFrame.width()) + u;
					if(!std::isnan(buf_depth[pixel_offset])) width++;
				}
			}

			std::shared_ptr<sensor_msgs::msg::PointCloud2> cloud_msg = std::make_shared<sensor_msgs::msg::PointCloud2>();

			cloud_msg->header.frame_id = name + "_cloud_tf_frame";
			cloud_msg->header.stamp = getSCtoROSTime(depthFrame);
			cloud_msg->height = 1;
			cloud_msg->width = width;
			cloud_msg->is_dense = false;
			cloud_msg->is_bigendian = false;


			sensor_msgs::PointCloud2Modifier pcd_modifier(*cloud_msg);
			pcd_modifier.setPointCloud2FieldsByString(1, "xyz");

			sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
			sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
			sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");

			for(int v = 0; v < depthFrame.height(); v++)
			{
				for(int u = 0; u < depthFrame.width(); u++)
				{

					std::size_t pixel_offset = (v*depthFrame.width()) + u;
					auto depth_in_meters = buf_depth[pixel_offset] * to_meters_multiplier;

					if(!std::isnan(depth_in_meters))
					{
						double x = depth_in_meters * (u - depth_intrinsics.cx) / depth_intrinsics.fx;
						double y = depth_in_meters * (v - depth_intrinsics.cy) / depth_intrinsics.fy;
						double z = depth_in_meters;

						*iter_x = z;
						*iter_y = -x;
						*iter_z = -y;

						++iter_x, ++iter_y, ++iter_z;
					}

				}
			}
			depth_points_callback(*cloud_msg);
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

		geometry_msgs::msg::TransformStamped createColorToDepthTf(ST::Matrix4 color_in_depth_frame, rclcpp::Time device_time)
		{
			ST::Vector4 rotQ = color_in_depth_frame.rotationAsQuaternion();
		    ST::Vector3f translation = color_in_depth_frame.translation();

		    geometry_msgs::msg::TransformStamped color_to_depth_tf;

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

		std::shared_ptr<sensor_msgs::msg::CameraInfo> populateCamInfo(const ST::Intrinsics &intrinics, rclcpp::Time device_time, std::string frame_name)
		{
			std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info = std::make_shared<sensor_msgs::msg::CameraInfo>();

			cam_info->header.stamp = device_time;
			cam_info->header.frame_id = frame_name;
			cam_info->height = intrinics.height;
			cam_info->width = intrinics.width;

			// No distortion
			cam_info->d.resize(5, 0);
			cam_info->distortion_model = sensor_msgs::distortion_models::PLUMB_BOB;

			cam_info->k[0] = intrinics.fx;
			cam_info->k[1] = 0;
			cam_info->k[2] = intrinics.cx;
			cam_info->k[3] = 0;
			cam_info->k[4] = intrinics.fy;
			cam_info->k[5] = intrinics.cy;
			cam_info->k[6] = 0;
			cam_info->k[7] = 0;
			cam_info->k[8] = 1;

			// No separate rectified image plane, so R = I
			cam_info->r.fill(0);
			cam_info->r[0] = cam_info->r[4] = cam_info->r[8] = 1;

			cam_info->p[0] = intrinics.fx;
			cam_info->p[1] = 0;
			cam_info->p[2] = intrinics.cx;
			cam_info->p[3] = 0;
			cam_info->p[4] = 0;
			cam_info->p[5] = intrinics.fy;
			cam_info->p[6] = intrinics.cy;
			cam_info->p[7] = 0;
			cam_info->p[8] = 0;
			cam_info->p[9] = 0;
			cam_info->p[10] = 1;
			cam_info->p[11] = 0;

			return cam_info;
		}
	};
}

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_ */
