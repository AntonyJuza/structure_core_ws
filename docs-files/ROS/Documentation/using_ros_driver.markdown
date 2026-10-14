# Using ROS1 for Structure Core {#ros_start}

<!-- more -->

# Launching ROS1 for Structure Core {#launch_ros1}

Use **roslaunch** in a bash terminal to start the Structure Core ROS driver: 

~~~{.sh}
roslaunch structure_core_ros_driver sc.launch
~~~

Use `rostopic echo` to check a topic's published data, e.g.

~~~
rostopic echo /sc/rgb/image
~~~

For more information on available topics, see [Topics and Parameters](@ref published_topics).

# Launching ROS1 for Structure Core with Rviz {#launch_ros1_rviz}

Run the following command to start ROS1 together with Rviz:

~~~{.sh}
roslaunch structure_core_ros_driver sc_rviz.launch
~~~

The driver will start publishing messages that will be visualized in Rviz.

![Caption text](rviz_new.png "Using RViz")

<br>

# Data Management {#data_management}

## Data Visualization using the image_view Package {#image_view}

Messages from running topics can also be visualized using the ROS package **image_view**. To display data using **image_view**, start two terminal instances. Launch the ROS1 in the first, and run the following command in the second:

~~~
rosrun image_view image_view image:=<topic name here>
~~~

For more information on available topics, see [Topics and Parameters](@ref published_topics).

![Caption text](image_view.png)

## Storing published data {#store_data}

Use the ROS package **rosbag** to store messages published by Structure Core ROS driver. To store all the data published by the ROS driver, run

~~~
rosbag record -a
~~~

...while the ROS driver is running. All published topics should be accumulated in a rosbag file. 

To store messages from specific topics use the following command:

~~~
rosbag record -O subset <topic to store>
~~~

To play data back use command:

~~~
rosbag play <bag file name>.bag
~~~

and stored messages will be published to the same topics as they were published by the ROS1


## Logging {#logging}

All ROS logs should be stored in `.ros/log/`. Check the logs if you have any issues launching the ROS1; they often help solve the issue.

<br>

# Integration with other packages {#integration}

## RGB-D point cloud construction with ROS depth_image_proc package

While RGB-D point cloud can be internally constructed and published by ROS1 for Structure Core, there is a possibility to do the same with external ROS packages.
Additional package that should be installed:

- **depth_image_proc**: sudo apt install ros-kinetic-depth-image-proc

ROS and ROS depth_image_proc nodes can be run using the following command:

~~~{.sh}
roslaunch structure_core_ros_driver depth_proc_rgbd_cloud.launch
~~~

## ROS Mapping and localization {#localization}

Using the data from Structure Core and additional ROS packages the task of mapping and localization can be acomplished.
Additional packages that should be installed:

- **imu_filter_madgwick**: sudo apt-get install ros-kinetic-imu-filter-madgwick
- **rtabmap_ros**: sudo apt-get install ros-kinetic-rtabmap-ros
- **robot_localization**: sudo apt-get install ros-kinetic-robot-localization

ROS localization and mapping modules can be run using the following command:

~~~{.sh}
roslaunch structure_core_ros_driver ros_odometry.launch
~~~

## OpenCV {#opencv}

In order to use ROS1 for Structure Core with OpenCV next packages should be installed:

- **opencv**:  sudo apt-get install ros-kinetic-opencv3
- **cv_bridge**:  sudo apt-get install ros-kinetic-cv-bridge

Sample code is located in **ros1/examples/opencvSubscriber.cpp**. It shows how to subscribe and display color and depth images using OpenCV and ROS1 for Structure Core.

To run the example node use the following command:

~~~{.sh}
rosrun structure_core_ros_driver opencv_subscriber
~~~


## PCL {#pcl}

In order to use ROS1 for Structure Core with PCL next packages should be installed:

- **PCL**:  sudo apt-get install ros-kinetic-pcl-ros
- **PCL_conversions**:  sudo apt-get install ros-kinetic-pcl-conversions

Sample code is located in **ros1/examples/pclSubscriber.cpp**. It shows how to subscribe  and display RGB-D point cloud using PCL and ROS1 for Structure Core.

To run the example node use the following command:

~~~{.sh}
rosrun structure_core_ros_driver pcl_subscriber
~~~

##  Cartographer ROS {#cartographer}

First Cartographer should be installed to the same workspace where ROS was installed using the steps described here:
[Cartographer building and installation](https://google-cartographer-ros.readthedocs.io/en/latest/compilation.html).

Cartographer can be used with the data recorded from Structure Core to bag file.

To create bag file run the following command:

~~~{.sh}
roslaunch structure_core_ros_driver sc_bag_for_cartographer.launch
~~~

Bag file will be created in `~/.ros/` directory.

After that Cartographer can be used with created bag file:

~~~{.sh}
roslaunch structure_core_ros_driver demo_sc_cartographer.launch bag_filename:=/path_to_bag_file
~~~

To run Cartographer in online mode use next command:

~~~{.sh}
roslaunch structure_core_ros_driver demo_sc_online_cartographer.launch 
~~~