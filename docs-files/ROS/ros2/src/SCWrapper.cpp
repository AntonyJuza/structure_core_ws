/*
    SCWrapper.cpp

    Copyright © 2019 Occipital, Inc. All rights reserved.
    This file is part of the Structure SDK.
    Unauthorized copying of this file, via any medium is strictly prohibited.
    Proprietary and confidential.

    http://structure.io
*/
#include <memory>
#include "rclcpp/rclcpp.hpp"
#include "structure_core_ros_driver/SCDriver.h"

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::executors::SingleThreadedExecutor exec;

  std::string ros_namespace = "sc";
  std::string node_name = "sc_node";
  bool intra_process_comms = false;

// SC main component
// Note: use the constructor to get node_name and namespace from the launch file
auto sc_node = std::make_shared<structure_core::Driver>(node_name, ros_namespace, intra_process_comms);

exec.add_node(sc_node);
exec.spin();
rclcpp::shutdown();
return 0;

}