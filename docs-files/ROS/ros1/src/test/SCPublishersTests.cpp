/*
 * SCPublishersTests.cpp
 *
 *  Created on: Mar 18, 2019
 *      Author: alexander
 */

#include "structure_core_ros_driver/SCDriver.h"

#include <memory>
#include <gtest/gtest.h>
#include <ros/ros.h>




TEST(DriverTest, driverStartStopTest)
{
	std::shared_ptr<structure_core::Driver> sc_driver = std::make_shared<structure_core::Driver>();

}


int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	ros::init(argc, argv, "tester");
	ros::NodeHandle nh;
	return RUN_ALL_TESTS();
}
