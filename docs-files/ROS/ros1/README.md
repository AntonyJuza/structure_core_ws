# Installation of the ROS1 for Structure Core

# 1) Prerequisites 

 - Ubuntu 16.04
 - ROS Kinetic
Structure Core SDK = 0.9

   or

 - Ubuntu 18.04
 - ROS Melodic
Structure Core SDK = 0.9

 All the dependencies for ROS and Structure Core SDK must also be installed.

 The ROS wrapper is available for both x86_64 and arm64 (aarch64) architectures.


# 2) Package Installation 

Open a bash terminal and initialize new **catkin** workspace, for example:

~~~{.sh}
cd 
mkdir -p catkin_ws/src 
cd ~/catkin_ws/src
~~~


Please copy SDK Cross-platform  folder with Strucrure Core ROS and ROS2 wrappers using the following commands:

~~~
cp StructureSDK-CrossPlatform-0.9
~~~

Set the ROS 1 version to use:

~~~
cd StructureSDK-CrossPlatform-0.9
~~~

Open **CMakeLists.txt** and on the first line set the version:

~~~
set(ROS_VERSION 1)
~~~

After that close the file and run **to_ros1.sh** script:

~~~
 chmod 755 to_ros1.sh
 source to_ros1.sh
~~~

Build the ROS wrapper using catkin:

~~~
cd ../../..
catkin_make
~~~

And finally initialize the ROS environment:

~~~
source ./devel/setup.bash
~~~
