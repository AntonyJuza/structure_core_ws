/*
    SCDriver.h

    Copyright © 2019 Occipital, Inc. All rights reserved.
    Updated for ROS 2 Jazzy.
*/

#ifndef INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_
#define INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include <image_transport/image_transport.hpp>
#include <image_transport/camera_publisher.hpp>
#include <tf2_ros/transform_broadcaster.h>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <geometry_msgs/msg/transform_stamped.hpp>

#include "structure_core_ros_driver/SCDevice.h"

namespace structure_core
{

class Driver : public rclcpp::Node
{
public:
    explicit Driver(const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
    Driver(const std::string & node_name, const rclcpp::NodeOptions & options = rclcpp::NodeOptions());
    virtual ~Driver();

    void onInit();
    void readParams();
    void connectCb();

    void onColorFrameCb(std::shared_ptr<sensor_msgs::msg::Image> vis_frame,
                        std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info);

    void onDepthFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame,
                        std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info,
                        geometry_msgs::msg::TransformStamped color_to_depth_tf);

    void onDepthAlignedFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame,
                               std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info);

    void onDepthPointsFrameCb(sensor_msgs::msg::PointCloud2 depth_points_frame);

    void onRGBDFrameCb(sensor_msgs::msg::PointCloud2 rgbd_frame);

    void onIrLeftFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame,
                         std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info);

    void onIrRightFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame,
                          std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info);

    void onIMUFrameCb(sensor_msgs::msg::Imu imu_msg);

    rcl_interfaces::msg::SetParametersResult onParameterChange(const std::vector<rclcpp::Parameter> & parameters);

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

    std::unique_ptr<tf2_ros::TransformBroadcaster> br_color_to_depth_tf;
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;

    std::mutex connect_mutex_;

    std::string name_ = "sc";
    bool visible_enable_ = true;
    bool visible_apply_gamma_correction_ = false;
    float visible_framerate_ = 30.0f;
    std::string visible_resolution_ = "Default";
    bool infrared_enable_ = false;
    bool infrared_left_enable_ = false;
    bool infrared_right_enable_ = false;
    float infrared_framerate_ = 30.0f;
    std::string infrared_mode_ = "Default";
    std::string infrared_resolution_ = "Default";
    bool infrared_disable_intensity_balance_ = true;
    bool imu_enable_ = false;
    std::string imu_update_rate_ = "Default";
    bool depth_enable_ = true;
    bool depth_aligned_enable_ = true;
    bool depth_pcloud_enable_ = false;
    float depth_framerate_ = 30.0f;
    std::string depth_resolution_ = "Default";
    bool depth_apply_correction_before_stream_ = false;
    float initial_projector_power_ = 1.0f;
    bool latency_reducer_enabled_ = true;
    int sensor_initialization_timeout_ = 20000;
    std::string sensor_serial_ = "null";
    std::string demosaic_method_ = "EdgeAware";
    bool frame_sync_enabled_ = true;
    bool low_latency_imu_ = false;
    bool rgbd_enable_ = true;

    // Dynamic parameters
    float visible_initial_gain_ = 2.0f;
    float visible_initial_exposure_ = 0.016f;
    bool infrared_auto_exposure_enabled_ = false;
    float infrared_initial_exposure_ = 0.0146f;
    float infrared_initial_gain_ = 3.0f;
    bool depth_apply_correction_ = true;
    int depth_range_mode_ = 1;
    int dynamic_calibration_mode_ = 0;

    bool sc_streaming = false;
};

} // namespace structure_core

#endif /* INCLUDE_STRUCTURE_CORE_ROS_DRIVER_SCDRIVER_H_ */
