#!/bin/bash

if [[ $1 = "BUILD_EXAMPLES" ]]; then
   sed -i 's/set(BUILD_EXAMPLES "FALSE")/set(BUILD_EXAMPLES "TRUE")/g' $PWD/ros2/CMakeLists.txt
else
  sed -i 's/set(BUILD_EXAMPLES "TRUE")/set(BUILD_EXAMPLES "FALSE")/g' $PWD/ros2/CMakeLists.txt
fi

sed -i 's/set(ROS_VERSION 1)/set(ROS_VERSION 2)/g' $PWD/CMakeLists.txt

#If the ros2 package xml was renamed, change it back. 
FILE=ros2/package.xml_ros2
if test -f "$FILE"; then
    mv  ros2/package.xml_ros2 ros2/package.xml
fi
#Renamed the ros1 package xml to avoid confusing ros build 
FILE2=ros1/package.xml
if test -f "$FILE2"; then
    mv  ros1/package.xml ros1/package.xml_ros1
fi
