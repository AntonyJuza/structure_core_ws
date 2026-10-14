/*
    SCtoSensorMsgConverter.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    Updated for ROS 2 Jazzy.
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <sensor_msgs/distortion_models.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <string>

#include <ST/CameraFrames.h>
#include <ST/Utilities.h>
#include "SCSession.h"

namespace structure_core
{

typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> ConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info, geometry_msgs::msg::TransformStamped color_to_depth_tf)> DepthConverterCallbackFunction;
typedef std::function<void(std::shared_ptr<sensor_msgs::msg::Image> image, std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)> DepthAlignedConverterCallbackFunction;
typedef std::function<void(sensor_msgs::msg::Imu imu_msg)> ConverterIMUCallbackFunction;
typedef std::function<void(sensor_msgs::msg::PointCloud2 depth_points)> ConverterDepthPointsCallbackFunction;
typedef std::function<void(sensor_msgs::msg::PointCloud2 rgbd_points)> ConverterRGBDCallbackFunction;

class Converter
{
private:
    ConverterCallbackFunction color_callback;
    DepthConverterCallbackFunction depth_callback;
    DepthAlignedConverterCallbackFunction depth_aligned_callback;
    ConverterDepthPointsCallbackFunction depth_points_callback;
    ConverterCallbackFunction ir_left_callback;
    ConverterCallbackFunction ir_right_callback;
    ConverterIMUCallbackFunction imu_callback;
    ConverterRGBDCallbackFunction rgbd_callback;

    rclcpp::Clock::SharedPtr clock_;
    std::string name_ = "sc";

public:
    Converter()
    : clock_(std::make_shared<rclcpp::Clock>(RCL_ROS_TIME))
    {
    }

    void setClock(rclcpp::Clock::SharedPtr clock)
    {
        clock_ = clock;
    }

    void setColorCallback(ConverterCallbackFunction callback) { color_callback = callback; }
    void setDepthCallback(DepthConverterCallbackFunction callback) { depth_callback = callback; }
    void setDepthAlignedCallback(DepthAlignedConverterCallbackFunction callback) { depth_aligned_callback = callback; }
    void setDepthPointsCallback(ConverterDepthPointsCallbackFunction callback) { depth_points_callback = callback; }
    void setRGBDCallback(ConverterRGBDCallbackFunction callback) { rgbd_callback = callback; }
    void setIrLeftCallback(ConverterCallbackFunction callback) { ir_left_callback = callback; }
    void setIrRightCallback(ConverterCallbackFunction callback) { ir_right_callback = callback; }
    void setIMUCallback(ConverterIMUCallbackFunction callback) { imu_callback = callback; }

    void setNamespace(const std::string & camera_name)
    {
        name_ = camera_name;
    }

    bool isMono(const ST::ColorFrame &visFrame)
    {
        return static_cast<size_t>(visFrame.width() * visFrame.height()) == visFrame.rgbSize();
    }

    std::string getEncoding(const ST::ColorFrame &visFrame)
    {
        return isMono(visFrame) ? sensor_msgs::image_encodings::MONO8 : sensor_msgs::image_encodings::RGB8;
    }

    int getStep(const ST::ColorFrame &visFrame)
    {
        return isMono(visFrame) ? 1 : 3;
    }

    template <typename M>
    rclcpp::Time getSCtoROSTime(M&& message)
    {
        if (!clock_) {
            clock_ = std::make_shared<rclcpp::Clock>(RCL_ROS_TIME);
        }
        double delta_sec = message.timestamp() - ST::getTimestampNow();
        return clock_->now() + rclcpp::Duration::from_seconds(delta_sec);
    }

    void onColorFrameAvailableEvent(const ST::ColorFrame &visFrame)
    {
        if (!color_callback) return;

        auto image = std::make_shared<sensor_msgs::msg::Image>();
        std::string frame_name = name_ + "_color_frame";
        rclcpp::Time stamp = getSCtoROSTime(visFrame);

        image->header.stamp = stamp;
        image->header.frame_id = frame_name;
        image->width = visFrame.width();
        image->height = visFrame.height();
        image->encoding = getEncoding(visFrame);
        image->is_bigendian = false;
        image->step = static_cast<sensor_msgs::msg::Image::_step_type>(getStep(visFrame) * image->width);
        size_t data_size = visFrame.rgbSize();
        image->data.resize(data_size);
        std::memcpy(image->data.data(), visFrame.rgbData(), data_size);

        auto cam_info = populateCamInfo(visFrame.intrinsics(), stamp, frame_name);
        color_callback(image, cam_info);
    }

    void onDepthFrameAvailableEvent(const ST::DepthFrame &depthFrame)
    {
        if (!depth_callback) return;

        auto image = std::make_shared<sensor_msgs::msg::Image>();
        std::string frame_name = name_ + "_depth_frame";
        rclcpp::Time stamp = getSCtoROSTime(depthFrame);

        image->header.stamp = stamp;
        image->header.frame_id = frame_name;
        image->width = depthFrame.width();
        image->height = depthFrame.height();
        image->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
        image->is_bigendian = false;
        image->step = static_cast<sensor_msgs::msg::Image::_step_type>(sizeof(float) * image->width);
        image->data.resize(image->step * image->height);

        const float* buf = depthFrame.depthInMillimeters();
        float* out_ptr = reinterpret_cast<float*>(image->data.data());
        const size_t total_pixels = static_cast<size_t>(depthFrame.width()) * depthFrame.height();
        constexpr float to_meters = 0.001f;

        for (size_t i = 0; i < total_pixels; ++i)
        {
            out_ptr[i] = buf[i] * to_meters;
        }

        auto cam_info = populateCamInfo(depthFrame.intrinsics(), stamp, frame_name);
        ST::Matrix4 color_in_depth_frame = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
        geometry_msgs::msg::TransformStamped color_to_depth_tf = createColorToDepthTf(color_in_depth_frame, stamp);

        depth_callback(image, cam_info, color_to_depth_tf);
    }

    void onDepthAlignedAvailableEvent(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
    {
        if (!depth_aligned_callback) return;

        auto image = std::make_shared<sensor_msgs::msg::Image>();
        std::string frame_name = name_ + "_depth_aligned_frame";
        rclcpp::Time stamp = getSCtoROSTime(depthFrame);

        image->header.stamp = stamp;
        image->header.frame_id = frame_name;
        image->width = visFrame.width();
        image->height = visFrame.height();
        image->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
        image->is_bigendian = false;
        image->step = static_cast<sensor_msgs::msg::Image::_step_type>(sizeof(float) * image->width);
        image->data.resize(image->step * image->height);

        float* registered_data = reinterpret_cast<float*>(image->data.data());
        const size_t total_registered = static_cast<size_t>(image->width) * image->height;
        std::fill(registered_data, registered_data + total_registered, std::numeric_limits<float>::quiet_NaN());

        const auto visible_from_depth = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
        ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();

        const ST::ColorFrame undistorted_color_frame = visFrame.undistorted();
        ST::Intrinsics color_intrinsics = undistorted_color_frame.intrinsics();

        const float* buf_depth = depthFrame.depthInMillimeters();
        constexpr double to_meters = 0.001;

        for (int y = 0; y < depthFrame.height(); ++y)
        {
            for (int x = 0; x < depthFrame.width(); ++x)
            {
                size_t pixel_offset = static_cast<size_t>(y * depthFrame.width() + x);
                float depth_mm = buf_depth[pixel_offset];

                if (!std::isnan(depth_mm) && depth_mm > 0.0f)
                {
                    double depth_m = depth_mm * to_meters;
                    int depth_point[2] = {x, y};
                    int visible_point[2] = {0, 0};
                    double depth_3d_point[3] = {0, 0, 0};

                    alignDepthPointToVisFrame(depth_point, visible_point, depth_3d_point, depth_m,
                                              depth_intrinsics, color_intrinsics, visible_from_depth);

                    if (visible_point[0] >= 0 && visible_point[0] < undistorted_color_frame.width() &&
                        visible_point[1] >= 0 && visible_point[1] < undistorted_color_frame.height())
                    {
                        size_t pixel_color_offset = static_cast<size_t>(visible_point[1] * undistorted_color_frame.width() + visible_point[0]);
                        registered_data[pixel_color_offset] = static_cast<float>(depth_m);
                    }
                }
            }
        }

        auto cam_info = populateCamInfo(depthFrame.intrinsics(), stamp, frame_name);
        depth_aligned_callback(image, cam_info);
    }

    void onIROneFrameAvailableEvent(const ST::InfraredFrame &irFrame, bool ir_left_image)
    {
        auto image = std::make_shared<sensor_msgs::msg::Image>();
        std::string frame_name = ir_left_image ? (name_ + "_ir_left_frame") : (name_ + "_ir_right_frame");
        rclcpp::Time stamp = getSCtoROSTime(irFrame);

        image->header.stamp = stamp;
        image->header.frame_id = frame_name;
        image->width = irFrame.width();
        image->height = irFrame.height();
        image->encoding = sensor_msgs::image_encodings::TYPE_16UC1;
        image->is_bigendian = false;
        image->step = static_cast<sensor_msgs::msg::Image::_step_type>(sizeof(uint16_t) * image->width);
        size_t data_size = image->step * image->height;
        image->data.resize(data_size);
        std::memcpy(image->data.data(), irFrame.data(), data_size);

        auto cam_info = populateCamInfo(irFrame.intrinsics(), stamp, frame_name);

        if (ir_left_image && ir_left_callback)
            ir_left_callback(image, cam_info);
        else if (!ir_left_image && ir_right_callback)
            ir_right_callback(image, cam_info);
    }

    void onIRBothFramesAvailableEvent(const ST::InfraredFrame &irFrame)
    {
        auto left_image = std::make_shared<sensor_msgs::msg::Image>();
        auto right_image = std::make_shared<sensor_msgs::msg::Image>();

        std::string left_frame_name = name_ + "_ir_left_frame";
        std::string right_frame_name = name_ + "_ir_right_frame";
        rclcpp::Time stamp = getSCtoROSTime(irFrame);

        int half_width = irFrame.width() / 2;
        int height = irFrame.height();

        left_image->header.stamp = stamp;
        left_image->header.frame_id = left_frame_name;
        left_image->width = half_width;
        left_image->height = height;
        left_image->encoding = sensor_msgs::image_encodings::MONO8;
        left_image->is_bigendian = false;
        left_image->step = half_width;
        left_image->data.resize(half_width * height);

        right_image->header.stamp = stamp;
        right_image->header.frame_id = right_frame_name;
        right_image->width = half_width;
        right_image->height = height;
        right_image->encoding = sensor_msgs::image_encodings::MONO8;
        right_image->is_bigendian = false;
        right_image->step = half_width;
        right_image->data.resize(half_width * height);

        const uint16_t* buf_ir = irFrame.data();

        for (int v = 0; v < height; ++v)
        {
            const uint16_t* row_src = buf_ir + v * irFrame.width();
            uint8_t* row_dst_r = right_image->data.data() + v * half_width;
            uint8_t* row_dst_l = left_image->data.data() + v * half_width;

            for (int u = 0; u < half_width; ++u)
            {
                row_dst_r[u] = static_cast<uint8_t>(row_src[u] >> 2);
                row_dst_l[u] = static_cast<uint8_t>(row_src[half_width + u] >> 2);
            }
        }

        auto left_info = populateCamInfo(irFrame.intrinsics(), stamp, left_frame_name);
        auto right_info = populateCamInfo(irFrame.intrinsics(), stamp, right_frame_name);

        if (ir_left_callback) ir_left_callback(left_image, left_info);
        if (ir_right_callback) ir_right_callback(right_image, right_info);
    }

    void onIMUAvailableEvent(const ST::AccelerometerEvent &accelEvent, const ST::GyroscopeEvent& gyroEvent)
    {
        if (!imu_callback) return;

        sensor_msgs::msg::Imu imu_msg;
        imu_msg.header.stamp = getSCtoROSTime(accelEvent);
        imu_msg.header.frame_id = name_ + "_imu_frame";

        // Structure Core accelerometer outputs in 'g', ROS standard is m/s^2 (1g = 9.80665 m/s^2)
        constexpr double g_to_mps2 = 9.80665;
        imu_msg.linear_acceleration.x = accelEvent.acceleration().x * g_to_mps2;
        imu_msg.linear_acceleration.y = accelEvent.acceleration().y * g_to_mps2;
        imu_msg.linear_acceleration.z = accelEvent.acceleration().z * g_to_mps2;

        // Structure Core gyroscope outputs in radians/second
        imu_msg.angular_velocity.x = gyroEvent.rotationRate().x;
        imu_msg.angular_velocity.y = gyroEvent.rotationRate().y;
        imu_msg.angular_velocity.z = gyroEvent.rotationRate().z;

        // Orientation is not estimated by raw driver; set orientation covariance[0] = -1 per REP-145
        imu_msg.orientation_covariance[0] = -1.0;

        imu_callback(imu_msg);
    }

    void onRGBDAvailableEvent(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
    {
        if (!rgbd_callback) return;

        const float* buf_depth = depthFrame.depthInMillimeters();
        const auto visible_from_depth = depthFrame.visibleCameraPoseInDepthCoordinateFrame();
        ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();

        const ST::ColorFrame undistorted_color_frame = visFrame.undistorted();
        ST::Intrinsics color_intrinsics = undistorted_color_frame.intrinsics();
        const uint8_t* buf_color = undistorted_color_frame.rgbData();
        bool mono = isMono(visFrame);

        int valid_count = 0;
        const int total_pixels = depthFrame.width() * depthFrame.height();
        for (int i = 0; i < total_pixels; ++i)
        {
            if (!std::isnan(buf_depth[i]) && buf_depth[i] > 0.0f)
                valid_count++;
        }

        sensor_msgs::msg::PointCloud2 cloud_msg;
        cloud_msg.header.frame_id = name_ + "_cloud_tf_frame";
        cloud_msg.header.stamp = getSCtoROSTime(depthFrame);
        cloud_msg.height = 1;
        cloud_msg.width  = valid_count;
        cloud_msg.is_dense = false;
        cloud_msg.is_bigendian = false;

        sensor_msgs::PointCloud2Modifier pcd_modifier(cloud_msg);
        pcd_modifier.setPointCloud2FieldsByString(2, "xyz", "rgb");

        sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
        sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");
        sensor_msgs::PointCloud2Iterator<uint8_t> iter_r(cloud_msg, "r");
        sensor_msgs::PointCloud2Iterator<uint8_t> iter_g(cloud_msg, "g");
        sensor_msgs::PointCloud2Iterator<uint8_t> iter_b(cloud_msg, "b");
        sensor_msgs::PointCloud2Iterator<uint8_t> iter_a(cloud_msg, "a");

        constexpr double to_meters = 0.001;

        for (int v = 0; v < depthFrame.height(); ++v)
        {
            for (int u = 0; u < depthFrame.width(); ++u)
            {
                size_t pixel_offset = static_cast<size_t>(v * depthFrame.width() + u);
                float depth_mm = buf_depth[pixel_offset];

                if (!std::isnan(depth_mm) && depth_mm > 0.0f)
                {
                    double depth_in_meters = depth_mm * to_meters;
                    int depth_point[2] = {u, v};
                    int visible_point[2] = {0, 0};
                    double depth_3d_point[3] = {0, 0, 0};

                    alignDepthPointToVisFrame(depth_point, visible_point, depth_3d_point, depth_in_meters,
                                              depth_intrinsics, color_intrinsics, visible_from_depth);

                    if (visible_point[0] >= 0 && visible_point[0] < undistorted_color_frame.width() &&
                        visible_point[1] >= 0 && visible_point[1] < undistorted_color_frame.height())
                    {
                        *iter_a = 255;
                        if (mono)
                        {
                            size_t c_offset = static_cast<size_t>(visible_point[1] * undistorted_color_frame.width() + visible_point[0]);
                            *iter_r = buf_color[c_offset];
                            *iter_g = buf_color[c_offset];
                            *iter_b = buf_color[c_offset];
                        }
                        else
                        {
                            size_t c_offset = static_cast<size_t>((visible_point[1] * undistorted_color_frame.width() + visible_point[0]) * 3);
                            *iter_r = buf_color[c_offset];
                            *iter_g = buf_color[c_offset + 1];
                            *iter_b = buf_color[c_offset + 2];
                        }

                        // Convert optical to ROS REP-103 body frame coordinates
                        *iter_x = static_cast<float>(depth_3d_point[2]);
                        *iter_y = static_cast<float>(-depth_3d_point[0]);
                        *iter_z = static_cast<float>(-depth_3d_point[1]);

                        ++iter_x; ++iter_y; ++iter_z;
                        ++iter_a; ++iter_r; ++iter_g; ++iter_b;
                    }
                }
            }
        }

        rgbd_callback(cloud_msg);
    }

    void onDepthPointsAvailableEvent(const ST::DepthFrame &depthFrame)
    {
        if (!depth_points_callback) return;

        const float* buf_depth = depthFrame.depthInMillimeters();
        ST::Intrinsics depth_intrinsics = depthFrame.intrinsics();

        int valid_count = 0;
        const int total_pixels = depthFrame.width() * depthFrame.height();
        for (int i = 0; i < total_pixels; ++i)
        {
            if (!std::isnan(buf_depth[i]) && buf_depth[i] > 0.0f)
                valid_count++;
        }

        sensor_msgs::msg::PointCloud2 cloud_msg;
        cloud_msg.header.frame_id = name_ + "_cloud_tf_frame";
        cloud_msg.header.stamp = getSCtoROSTime(depthFrame);
        cloud_msg.height = 1;
        cloud_msg.width = valid_count;
        cloud_msg.is_dense = false;
        cloud_msg.is_bigendian = false;

        sensor_msgs::PointCloud2Modifier pcd_modifier(cloud_msg);
        pcd_modifier.setPointCloud2FieldsByString(1, "xyz");

        sensor_msgs::PointCloud2Iterator<float> iter_x(cloud_msg, "x");
        sensor_msgs::PointCloud2Iterator<float> iter_y(cloud_msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(cloud_msg, "z");

        constexpr double to_meters = 0.001;

        for (int v = 0; v < depthFrame.height(); ++v)
        {
            for (int u = 0; u < depthFrame.width(); ++u)
            {
                size_t pixel_offset = static_cast<size_t>(v * depthFrame.width() + u);
                float depth_mm = buf_depth[pixel_offset];

                if (!std::isnan(depth_mm) && depth_mm > 0.0f)
                {
                    double z = depth_mm * to_meters;
                    double x = z * (u - depth_intrinsics.cx) / depth_intrinsics.fx;
                    double y = z * (v - depth_intrinsics.cy) / depth_intrinsics.fy;

                    *iter_x = static_cast<float>(z);
                    *iter_y = static_cast<float>(-x);
                    *iter_z = static_cast<float>(-y);

                    ++iter_x; ++iter_y; ++iter_z;
                }
            }
        }

        depth_points_callback(cloud_msg);
    }

    void alignDepthPointToVisFrame(const int *depth_point, int *visible_point, double *depth_3d_point, const double depth_in_meters,
                                  const ST::Intrinsics &depth_intrinsics, const ST::Intrinsics &color_intrinsics, const ST::Matrix4 &depth_color_transform)
    {
        depth_3d_point[0] = depth_in_meters * (depth_point[0] - depth_intrinsics.cx) / depth_intrinsics.fx;
        depth_3d_point[1] = depth_in_meters * (depth_point[1] - depth_intrinsics.cy) / depth_intrinsics.fy;
        depth_3d_point[2] = depth_in_meters;

        ST::Vector3f xyz_depth(static_cast<float>(depth_3d_point[0]),
                               static_cast<float>(depth_3d_point[1]),
                               static_cast<float>(depth_3d_point[2]));

        auto xyz_visible = depth_color_transform * xyz_depth;

        float float_visible_col = (xyz_visible.x * color_intrinsics.fx / xyz_visible.z) + color_intrinsics.cx;
        float float_visible_row = (xyz_visible.y * color_intrinsics.fy / xyz_visible.z) + color_intrinsics.cy;

        visible_point[0] = static_cast<int>(std::round(float_visible_col));
        visible_point[1] = static_cast<int>(std::round(float_visible_row));
    }

    geometry_msgs::msg::TransformStamped createColorToDepthTf(const ST::Matrix4 &color_in_depth_frame, rclcpp::Time stamp)
    {
        ST::Vector4 rotQ = color_in_depth_frame.rotationAsQuaternion();
        ST::Vector3f translation = color_in_depth_frame.translation();

        geometry_msgs::msg::TransformStamped color_to_depth_tf;
        color_to_depth_tf.header.stamp = stamp;
        color_to_depth_tf.header.frame_id = name_ + "_depth_frame";
        color_to_depth_tf.child_frame_id = name_ + "_color_frame";

        color_to_depth_tf.transform.translation.x = translation.x;
        color_to_depth_tf.transform.translation.y = translation.y;
        color_to_depth_tf.transform.translation.z = translation.z;

        color_to_depth_tf.transform.rotation.x = rotQ.x;
        color_to_depth_tf.transform.rotation.y = rotQ.y;
        color_to_depth_tf.transform.rotation.z = rotQ.z;
        color_to_depth_tf.transform.rotation.w = rotQ.w;

        return color_to_depth_tf;
    }

    std::shared_ptr<sensor_msgs::msg::CameraInfo> populateCamInfo(const ST::Intrinsics &intrinsics, rclcpp::Time stamp, const std::string &frame_name)
    {
        auto cam_info = std::make_shared<sensor_msgs::msg::CameraInfo>();
        cam_info->header.stamp = stamp;
        cam_info->header.frame_id = frame_name;
        cam_info->height = intrinsics.height;
        cam_info->width = intrinsics.width;

        cam_info->distortion_model = sensor_msgs::distortion_models::PLUMB_BOB;
        cam_info->d.resize(5, 0.0);

        cam_info->k[0] = intrinsics.fx;
        cam_info->k[1] = 0.0;
        cam_info->k[2] = intrinsics.cx;
        cam_info->k[3] = 0.0;
        cam_info->k[4] = intrinsics.fy;
        cam_info->k[5] = intrinsics.cy;
        cam_info->k[6] = 0.0;
        cam_info->k[7] = 0.0;
        cam_info->k[8] = 1.0;

        cam_info->r.fill(0.0);
        cam_info->r[0] = 1.0;
        cam_info->r[4] = 1.0;
        cam_info->r[8] = 1.0;

        cam_info->p[0] = intrinsics.fx;
        cam_info->p[1] = 0.0;
        cam_info->p[2] = intrinsics.cx;
        cam_info->p[3] = 0.0;
        cam_info->p[4] = 0.0;
        cam_info->p[5] = intrinsics.fy;
        cam_info->p[6] = intrinsics.cy;
        cam_info->p[7] = 0.0;
        cam_info->p[8] = 0.0;
        cam_info->p[9] = 0.0;
        cam_info->p[10] = 1.0;
        cam_info->p[11] = 0.0;

        return cam_info;
    }
};

// Implement SessionDelegate listener attachment and notifications
inline void SessionDelegate::attachListener(std::shared_ptr<Converter> deviceListener)
{
    device_listeners.push_back(deviceListener);
}

inline void SessionDelegate::notifyIRListeners(const ST::InfraredFrame &infraredFrame)
{
    for (auto & listener : device_listeners)
    {
        if (img_proc_settings.ir_both_cameras)
            listener->onIRBothFramesAvailableEvent(infraredFrame);
        else
            listener->onIROneFrameAvailableEvent(infraredFrame, img_proc_settings.ir_left_camera);
    }
}

inline void SessionDelegate::notifyDepthListeners(const ST::DepthFrame &depthFrame)
{
    for (auto & listener : device_listeners)
    {
        listener->onDepthFrameAvailableEvent(depthFrame);
        if (img_proc_settings.depth_pcloud_enable_)
            listener->onDepthPointsAvailableEvent(depthFrame);
    }
}

inline void SessionDelegate::notifyColorListeners(const ST::ColorFrame &visFrame)
{
    for (auto & listener : device_listeners)
    {
        listener->onColorFrameAvailableEvent(visFrame);
    }
}

inline void SessionDelegate::notifyRGBDListeners(const ST::DepthFrame &depthFrame, const ST::ColorFrame &visFrame)
{
    for (auto & listener : device_listeners)
    {
        if (img_proc_settings.rgbd_enable_)
            listener->onRGBDAvailableEvent(depthFrame, visFrame);
        if (img_proc_settings.depth_aligned_enable_)
            listener->onDepthAlignedAvailableEvent(depthFrame, visFrame);
    }
}

inline void SessionDelegate::notifyIMUListeners(const ST::AccelerometerEvent &accel, const ST::GyroscopeEvent& gyro)
{
    for (auto & listener : device_listeners)
    {
        listener->onIMUAvailableEvent(accel, gyro);
    }
}

} // namespace structure_core

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCTOSENSORMSGCONVERTER_H_ */
