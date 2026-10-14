# Installation of the ROS2 for Structure Core

<!-- more -->

# 1) Prerequisites 

 - Ubuntu 18.04
 - ROS Crystal Clemmys
Structure Core SDK = 0.9

 All the dependencies for ROS and Structure Core SDK must also be installed.

 The ROS wrapper is available for both x86_64 and arm64 (aarch64) architectures.


# 2) Package Installation 

Open a bash terminal and initialize new **ROS2** workspace, for example:

~~~{.sh}
cd 
mkdir -p colcon_ws/src 
cd ~/colcon_ws/src
~~~

Please copy SDK Cross-platform  folder with Strucrure Core ROS and ROS2 wrappers using the following command:

~~~
cp StructureSDK-CrossPlatform-0.9
~~~

Download image_common package for ROS2:

~~~
git clone https://github.com/ros-perception/image_common.git
cd image_common
git fetch origin ros2:ros2
git checkout ros2
git checkout eb4680dcfa35fc47e5f0329916a37d743323fd6c
~~~

Set the ROS 2 version to use:

~~~
cd StructureSDK-CrossPlatform-0.9
~~~

Open **CMakeLists.txt** and on the first line set the version:

~~~
set(ROS_VERSION 2)
~~~

After that close the file and run **to_ros2.sh** script:

~~~
chmod 755 to_ros2.sh
source to_ros2.sh
~~~

Build the ROS2 wrapper using colcon:

~~~
cp -r ros2_driver/ ../..
cd ../../..
colcon build --symlink-install
~~~
Colcon supports the option **--symlink-install**. This allows the installed files to be changed by changing the files in the source space (e.g. Python files or other not compiled resourced) for faster iteration.

And finally initialize the ROS environment:

~~~
source ./install/setup.bash
~~~
