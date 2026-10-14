# Installation (ROS1) {#ros_install}

<!-- more -->

# Prerequisites {#prerequisites}

 - Ubuntu 16.04
 - ROS Kinetic
Structure Core SDK = 0.9

   or

 - Ubuntu 18.04
 - ROS Melodic
Structure Core SDK = 0.9

 All dependencies for ROS1 and Structure Core SDK must also be installed. The ROS1 wrapper is compatible with x86_64 and arm64 (arch64) architectures.

# Package Installation {#package_installation}

Open a bash terminal and initialize a new **catkin** workspace. Copy the SDK Cross-platform folder into this workspace.

~~~{.sh}
cd 
mkdir -p catkin_ws/src 
cd ~/catkin_ws/src
cp StructureSDK-CrossPlatform-0.9
cd StructureSDK-CrossPlatform-0.9
~~~

Run the **to_ros1.sh** script.

~~~{.sh}
 chmod 755 to_ros1.sh
 source to_ros1.sh
~~~

How to build and install examples can follow (@ref integration) and then run following commands :

~~~{.sh}
 chmod 755 to_ros1.sh
 source to_ros1.sh BUILD_EXAMPLES
~~~

Use catkin to build the ROS1 for Structure Core at the top of the directory.

~~~{.sh}
cd ../../..
catkin_make
~~~

And finally, initialize your ROS environment:

~~~
source ./devel/setup.bash
~~~
