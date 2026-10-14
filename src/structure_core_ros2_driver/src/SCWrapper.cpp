/*
    SCWrapper.cpp

    Copyright © 2019 Occipital, Inc. All rights reserved.
    Updated for ROS 2 Jazzy.
*/

#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "structure_core_ros_driver/SCDriver.h"

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);

    rclcpp::NodeOptions options;
    options.allow_undeclared_parameters(true);
    options.automatically_declare_parameters_from_overrides(false);

    auto sc_node = std::make_shared<structure_core::Driver>(options);

    rclcpp::spin(sc_node);
    rclcpp::shutdown();
    return 0;
}