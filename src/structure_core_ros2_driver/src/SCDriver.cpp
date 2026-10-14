/*
    SCDriver.cpp

    Copyright © 2019 Occipital, Inc. All rights reserved.
    Updated for ROS 2 Jazzy.
*/

#include "structure_core_ros_driver/SCDriver.h"
#include <rclcpp_components/register_node_macro.hpp>
#include <iostream>

namespace structure_core
{

Driver::Driver(const rclcpp::NodeOptions & options)
: Node("sc_node", options)
{
    onInit();
}

Driver::Driver(const std::string & node_name, const rclcpp::NodeOptions & options)
: Node(node_name, options)
{
    onInit();
}

Driver::~Driver()
{
    if (device && sc_streaming)
    {
        RCLCPP_INFO(get_logger(), "Stopping Structure Core stream...");
        device->stopStream();
        sc_streaming = false;
    }
}

void Driver::onInit()
{
    RCLCPP_INFO(get_logger(), "Initializing Structure Core ROS 2 Driver...");

    device = std::make_shared<structure_core::CameraDevice>();

    readParams();

    // Set clock from this node
    device->setClock(this->get_clock());

    // Initialize TF broadcaster
    br_color_to_depth_tf = std::make_unique<tf2_ros::TransformBroadcaster>(*this);

    // Register dynamic parameters callback
    param_callback_handle_ = this->add_on_set_parameters_callback(
        std::bind(&Driver::onParameterChange, this, std::placeholders::_1));

    std::lock_guard<std::mutex> lock(connect_mutex_);

    if (visible_enable_)
    {
        pub_color = image_transport::create_camera_publisher(this, "rgb/image");
        RCLCPP_INFO(get_logger(), "Visible camera stream enabled: rgb/image");
    }

    if (depth_enable_)
    {
        pub_depth = image_transport::create_camera_publisher(this, "depth/image");
        RCLCPP_INFO(get_logger(), "Depth camera stream enabled: depth/image");

        if (depth_pcloud_enable_)
        {
            pub_depth_points = create_publisher<sensor_msgs::msg::PointCloud2>("depth/points", 10);
            RCLCPP_INFO(get_logger(), "Depth point cloud enabled: depth/points");
        }
    }

    if (depth_aligned_enable_)
    {
        pub_depth_aligned = image_transport::create_camera_publisher(this, "depth_aligned/image");
        RCLCPP_INFO(get_logger(), "Aligned depth camera stream enabled: depth_aligned/image");
    }

    if (rgbd_enable_)
    {
        pub_rgbd = create_publisher<sensor_msgs::msg::PointCloud2>("rgbd/points", 10);
        RCLCPP_INFO(get_logger(), "RGB-D colored point cloud enabled: rgbd/points");
    }

    if (infrared_enable_ && infrared_left_enable_)
    {
        pub_ir_left = image_transport::create_camera_publisher(this, "depth/infrared_left");
        RCLCPP_INFO(get_logger(), "Left IR camera stream enabled: depth/infrared_left");
    }

    if (infrared_enable_ && infrared_right_enable_)
    {
        pub_ir_right = image_transport::create_camera_publisher(this, "depth/infrared_right");
        RCLCPP_INFO(get_logger(), "Right IR camera stream enabled: depth/infrared_right");
    }

    if (imu_enable_)
    {
        pub_imu = create_publisher<sensor_msgs::msg::Imu>("imu/data_raw", 100);
        RCLCPP_INFO(get_logger(), "IMU stream enabled: imu/data_raw");
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

template<typename T>
static T declare_or_get(rclcpp::Node * node, const std::string & name, const T & default_val)
{
    if (node->has_parameter(name))
    {
        T val;
        node->get_parameter(name, val);
        return val;
    }
    return node->declare_parameter<T>(name, default_val);
}

void Driver::readParams()
{
    // Visible sensor parameters
    visible_enable_ = declare_or_get<bool>(this, "visible_enable", true);
    visible_apply_gamma_correction_ = declare_or_get<bool>(this, "visible_apply_gamma_correction", false);
    visible_framerate_ = static_cast<float>(declare_or_get<double>(this, "visible_framerate", 30.0));
    visible_resolution_ = declare_or_get<std::string>(this, "visible_resolution", "Default");

    // Infrared parameters
    infrared_enable_ = declare_or_get<bool>(this, "infrared_enable", false);
    infrared_framerate_ = static_cast<float>(declare_or_get<double>(this, "infrared_framerate", 30.0));
    infrared_mode_ = declare_or_get<std::string>(this, "infrared_mode", "Default");
    infrared_resolution_ = declare_or_get<std::string>(this, "infrared_resolution", "Default");
    infrared_disable_intensity_balance_ = declare_or_get<bool>(this, "infrared_disable_intensity_balance", true);

    // IMU parameters
    imu_enable_ = declare_or_get<bool>(this, "imu_enable", false);
    imu_update_rate_ = declare_or_get<std::string>(this, "imu_update_rate", "Default");

    // Depth parameters
    depth_enable_ = declare_or_get<bool>(this, "depth_enable", true);
    depth_aligned_enable_ = declare_or_get<bool>(this, "depth_aligned_enable", true);
    depth_pcloud_enable_ = declare_or_get<bool>(this, "depth_pcloud_enable", false);
    depth_framerate_ = static_cast<float>(declare_or_get<double>(this, "depth_framerate", 30.0));
    depth_resolution_ = declare_or_get<std::string>(this, "depth_resolution", "Default");
    depth_apply_correction_before_stream_ = declare_or_get<bool>(this, "depth_apply_correction_before_stream", false);

    // Processing and sensor settings
    demosaic_method_ = declare_or_get<std::string>(this, "demosaic_method", "EdgeAware");
    initial_projector_power_ = static_cast<float>(declare_or_get<double>(this, "initial_projector_power", 1.0));
    latency_reducer_enabled_ = declare_or_get<bool>(this, "latency_reducer_enabled", true);
    sensor_initialization_timeout_ = declare_or_get<int>(this, "sensor_initialization_timeout", 20000);
    sensor_serial_ = declare_or_get<std::string>(this, "sensor_serial", "null");
    frame_sync_enabled_ = declare_or_get<bool>(this, "frame_sync_enabled", true);
    low_latency_imu_ = declare_or_get<bool>(this, "low_latency_imu", false);
    rgbd_enable_ = declare_or_get<bool>(this, "rgbd_enable", true);

    name_ = declare_or_get<std::string>(this, "name", "sc");

    // Dynamic parameters
    visible_initial_gain_ = static_cast<float>(declare_or_get<double>(this, "visible_initial_gain", 2.0));
    visible_initial_exposure_ = static_cast<float>(declare_or_get<double>(this, "visible_initial_exposure", 0.016));
    infrared_auto_exposure_enabled_ = declare_or_get<bool>(this, "infrared_auto_exposure_enabled", false);
    infrared_initial_exposure_ = static_cast<float>(declare_or_get<double>(this, "infrared_initial_exposure", 0.0146));
    infrared_initial_gain_ = static_cast<float>(declare_or_get<double>(this, "infrared_initial_gain", 3.0));
    depth_apply_correction_ = declare_or_get<bool>(this, "depth_apply_correction", true);
    depth_range_mode_ = declare_or_get<int>(this, "depth_range_mode", 1);
    dynamic_calibration_mode_ = declare_or_get<int>(this, "dynamic_calibration_mode", 0);

    if (infrared_mode_ == "RightCameraOnly")
    {
        infrared_right_enable_ = true;
        infrared_left_enable_ = false;
    }
    else if (infrared_mode_ == "LeftCameraOnly")
    {
        infrared_left_enable_ = true;
        infrared_right_enable_ = false;
    }
    else
    {
        infrared_right_enable_ = true;
        infrared_left_enable_ = true;
    }

    if (rgbd_enable_)
    {
        visible_enable_ = true;
        depth_enable_ = true;
    }

    device->setParams(
        visible_enable_,
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
        name_
    );
}

rcl_interfaces::msg::SetParametersResult Driver::onParameterChange(const std::vector<rclcpp::Parameter> & parameters)
{
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "success";

    bool update_dyn_params = false;

    for (const auto & param : parameters)
    {
        const auto & pname = param.get_name();

        if (pname == "visible_initial_gain")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE &&
                param.as_double() >= 1.0 && param.as_double() <= 8.0)
            {
                visible_initial_gain_ = static_cast<float>(param.as_double());
                update_dyn_params = true;
            }
            else
            {
                RCLCPP_WARN(get_logger(), "visible_initial_gain must be in [1.0, 8.0]");
                result.successful = false;
                result.reason = "visible_initial_gain out of range";
                return result;
            }
        }
        else if (pname == "visible_initial_exposure")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE &&
                param.as_double() >= 0.0 && param.as_double() <= 0.03)
            {
                visible_initial_exposure_ = static_cast<float>(param.as_double());
                update_dyn_params = true;
            }
            else
            {
                RCLCPP_WARN(get_logger(), "visible_initial_exposure must be in [0.0, 0.03]");
                result.successful = false;
                result.reason = "visible_initial_exposure out of range";
                return result;
            }
        }
        else if (pname == "infrared_auto_exposure_enabled")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_BOOL)
            {
                infrared_auto_exposure_enabled_ = param.as_bool();
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "infrared_auto_exposure_enabled must be bool";
                return result;
            }
        }
        else if (pname == "infrared_initial_exposure")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE &&
                param.as_double() >= 0.0 && param.as_double() <= 0.03)
            {
                infrared_initial_exposure_ = static_cast<float>(param.as_double());
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "infrared_initial_exposure out of range";
                return result;
            }
        }
        else if (pname == "infrared_initial_gain")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_DOUBLE &&
                param.as_double() >= 0.0 && param.as_double() <= 3.0)
            {
                infrared_initial_gain_ = static_cast<float>(param.as_double());
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "infrared_initial_gain out of range";
                return result;
            }
        }
        else if (pname == "depth_apply_correction")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_BOOL)
            {
                depth_apply_correction_ = param.as_bool();
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "depth_apply_correction must be bool";
                return result;
            }
        }
        else if (pname == "depth_range_mode")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_INTEGER &&
                param.as_int() >= -1 && param.as_int() <= 5)
            {
                depth_range_mode_ = static_cast<int>(param.as_int());
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "depth_range_mode out of range [-1, 5]";
                return result;
            }
        }
        else if (pname == "dynamic_calibration_mode")
        {
            if (param.get_type() == rclcpp::ParameterType::PARAMETER_INTEGER &&
                param.as_int() >= 0 && param.as_int() <= 2)
            {
                dynamic_calibration_mode_ = static_cast<int>(param.as_int());
                update_dyn_params = true;
            }
            else
            {
                result.successful = false;
                result.reason = "dynamic_calibration_mode out of range [0, 2]";
                return result;
            }
        }
    }

    if (update_dyn_params && device && device->sessionIsInit())
    {
        bool ok = device->setNewDynParams(
            visible_initial_gain_,
            visible_initial_exposure_,
            infrared_auto_exposure_enabled_,
            infrared_initial_exposure_,
            infrared_initial_gain_,
            depth_apply_correction_,
            depth_range_mode_,
            dynamic_calibration_mode_
        );
        result.successful = ok;
    }

    return result;
}

void Driver::connectCb()
{
    RCLCPP_INFO(get_logger(), "Starting Structure Core streaming...");
    sc_streaming = true;
    device->startStream();
}

void Driver::onColorFrameCb(std::shared_ptr<sensor_msgs::msg::Image> vis_frame,
                            std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
{
    pub_color.publish(*vis_frame, *cam_info);
}

void Driver::onDepthFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame,
                            std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info,
                            geometry_msgs::msg::TransformStamped color_to_depth_tf)
{
    pub_depth.publish(*depth_frame, *cam_info);

    if (br_color_to_depth_tf)
    {
        br_color_to_depth_tf->sendTransform(color_to_depth_tf);
    }
}

void Driver::onDepthAlignedFrameCb(std::shared_ptr<sensor_msgs::msg::Image> depth_frame,
                                   std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
{
    pub_depth_aligned.publish(*depth_frame, *cam_info);
}

void Driver::onDepthPointsFrameCb(sensor_msgs::msg::PointCloud2 depth_points_frame)
{
    if (pub_depth_points)
    {
        pub_depth_points->publish(depth_points_frame);
    }
}

void Driver::onRGBDFrameCb(sensor_msgs::msg::PointCloud2 rgbd_frame)
{
    if (pub_rgbd)
    {
        pub_rgbd->publish(rgbd_frame);
    }
}

void Driver::onIrLeftFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame,
                             std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
{
    pub_ir_left.publish(*ir_frame, *cam_info);
}

void Driver::onIrRightFrameCb(std::shared_ptr<sensor_msgs::msg::Image> ir_frame,
                              std::shared_ptr<sensor_msgs::msg::CameraInfo> cam_info)
{
    pub_ir_right.publish(*ir_frame, *cam_info);
}

void Driver::onIMUFrameCb(sensor_msgs::msg::Imu imu_msg)
{
    if (pub_imu)
    {
        pub_imu->publish(imu_msg);
    }
}

} // namespace structure_core

RCLCPP_COMPONENTS_REGISTER_NODE(structure_core::Driver)