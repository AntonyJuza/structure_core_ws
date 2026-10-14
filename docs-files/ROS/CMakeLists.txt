set(ROS_VERSION 1)

cmake_minimum_required(VERSION 3.5)

if(${ROS_VERSION} STREQUAL "2")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ros1)

if(${ROS_VERSION} STREQUAL "2")
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ros2)

endif()
