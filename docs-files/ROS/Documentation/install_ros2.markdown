# Installation (ROS2) {#ros2_install}

<!-- more -->

# Prerequisites {#prerequisites2}

 - Ubuntu 18.04
 - ROS Crystal Clemmys
Structure Core SDK = 0.9

 All dependencies for ROS and Structure Core SDK must also be installed. The ROS2 wrapper is available for both x86_64 and arm64 (arch64) architectures.

# Package Installation {#package2_installation}

Open a bash terminal and initialize new **ROS2** workspace. Copy the SDK Cross-platform folder into this workspace.

~~~{.sh}
cd 
mkdir -p colcon_ws/src 
cd ~/colcon_ws/src
cp StructureSDK-CrossPlatform-0.9
~~~

Download the `image_common` package for ROS2.

~~~{.sh}
git clone https://github.com/ros-perception/image_common.git
cd image_common
git fetch origin ros2:ros2
git checkout ros2
git checkout eb4680dcfa35fc47e5f0329916a37d743323fd6c

cd ../StructureSDK-CrossPlatform-0.9
~~~

Run the **to_ros2.sh** script.

~~~{.sh}
chmod 755 to_ros2.sh
source to_ros2.sh
~~~

Use colcon to build the ROS2 for Structure Core. Colcon supports the option `--symlink-install`, which allows the installed files to be changed in the source space (e.g. Python files or other not compiled resourced) for faster iteration.

You will need to copy ros2 folder under colcon_ws/src and change
SCSDK_ROOT path toward SDK ros2/CMakeLists.txt e.g:
set(SCSDK_ROOT ${PARENT_DIR}/StructureSDK-CrossPlatform-0.7.3/)

~~~{.sh}
cp -r ros2/ ../..
cd ../../..
colcon build --symlink-install
~~~

And finally, initialize your ROS environment:

~~~
source ./install/setup.bash
~~~
