# Using ROS2 for Structure Core {#ros2_start}

<!-- more -->

# Launching ROS2 for Structure Core {#launch_ros2}

Use **ros2 launch** in a bash terminal to start:

~~~{.sh}
ros2 launch structure_core_ros2_driver sc.launch.py
~~~

To check the data that ROS driver publishes use the following command:

~~~
ros2 topic echo <topic name here>
~~~

# Launching ROS2 for Structure Core with Rviz {#launch_ros2_rviz2}

To start the ROS2 together with Rviz use the following command:

~~~{.sh}
ros2 launch structure_core_ros2_driver sc_rviz2.launch.py
~~~

This will start publishing messages that will be visualized in Rviz. An example visualization is below: 

![ROS + RViz](rviz2.png)

