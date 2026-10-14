#!/bin/bash

if [[ $1 = "BUILD_EXAMPLES" ]]; then
   sed -i 's/set(BUILD_EXAMPLES "FALSE")/set(BUILD_EXAMPLES "TRUE")/g' $PWD/ros1/CMakeLists.txt
else
  sed -i 's/set(BUILD_EXAMPLES "TRUE")/set(BUILD_EXAMPLES "FALSE")/g' $PWD/ros1/CMakeLists.txt
fi

sed -i 's/set(ROS_VERSION 2)/set(ROS_VERSION 1)/g' $PWD/CMakeLists.txt

#If ros1 package xml was renamed, change it back
FILE=ros1/package.xml_ros1
if test -f "$FILE"; then
    mv  ros1/package.xml_ros1 ros1/package.xml
fi

#Renamed the ros2 package xml to avoid confusing ros build 
FILE2=ros2/package.xml
if test -f "$FILE2"; then
    mv  ros2/package.xml ros2/package.xml_ros2
fi
