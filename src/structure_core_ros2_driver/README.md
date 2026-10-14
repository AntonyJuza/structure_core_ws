# Structure Core ROS 2 Jazzy Driver

A modernized, high-performance ROS 2 driver package for the **Occipital Structure Core** depth and infrared camera, fully upgraded and verified for **ROS 2 Jazzy Jalisco on Ubuntu 24.04 LTS (Noble)**.

---

## 1. Key Updates for ROS 2 Jazzy

This package is a rewrite and modernization of the early ROS 2 (Crystal/Dashing) Structure Core driver:
- **ROS 2 Jazzy Standard Node Implementation**: Replaced deprecated `Node(name, namespace, intra_process)` constructor with `rclcpp::NodeOptions`.
- **Dynamic Parameter API**: Upgraded obsolete `register_param_change_callback` to modern `add_on_set_parameters_callback` with `declare_or_get` pattern to prevent `ParameterAlreadyDeclaredException`.
- **Standard ROS 2 Launch Files**: Replaced deprecated `node_executable`, `node_name`, `node_namespace` with `executable`, `name`, `namespace`. Updated `static_transform_publisher` to required `--frame-id` and `--child-frame-id` syntax.
- **REP-103 IMU Units**: Properly converts accelerometer readings from raw sensor `g` into SI units ($m/s^2$, $1g = 9.80665 m/s^2$) with standard `/sc/imu/data_raw` naming.
- **Fast Zero-Copy PointCloud & Image Conversion**: Removed stack allocation of 5MB arrays and slow repeated `std::vector::push_back` in IR/Depth conversion routines.
- **Self-Contained Structure SDK**: Includes prebuilt Structure SDK headers and dynamic library (`libStructure.so`) with automatic `RPATH` linkage and CMake detection.

---

## 2. Prerequisites & USB Permission Setup

Structure Core communicates over userspace USB through `libusb` and requires non-root read/write permissions.

### Install Udev Rules (One-time setup):
Run the provided installer script:
```bash
sudo ./src/structure_core_ros2_driver/scripts/install_udev_rules.sh
sudo udevadm control --reload-rules && sudo udevadm trigger
```

Alternatively, you can temporarily grant write access to the USB device node:
```bash
# Find USB bus and device:
lsusb -d 277a:
# e.g., if Bus 002 Device 002:
sudo chmod 666 /dev/bus/usb/002/002
```

---

## 3. Building the Workspace

```bash
cd /media/juza/8AE62852E62840B5/PC/Home/depth_ws
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select structure_core_ros2_driver
source install/setup.bash
```

---

## 4. Running the Driver

### Option A: Using the Convenience Script
```bash
# Run camera driver
./start_structure_core.sh

# Run camera driver + RViz2 visualization
./start_structure_core.sh --rviz
```

### Option B: Using ROS 2 Launch Directly
```bash
source install/setup.bash

# Basic launch
ros2 launch structure_core_ros2_driver sc.launch.py

# Launch with RViz2 visualization
ros2 launch structure_core_ros2_driver sc.launch.py use_rviz:=true

# Or use the designated RViz launch file
ros2 launch structure_core_ros2_driver sc_rviz2.launch.py
```

---

## 5. Published Topics

| Topic | Type | Description |
|---|---|---|
| `/sc/rgb/image` | `sensor_msgs/msg/Image` | Color RGB8 / MONO8 image from visible sensor |
| `/sc/rgb/camera_info` | `sensor_msgs/msg/CameraInfo` | Visible camera intrinsics |
| `/sc/depth/image` | `sensor_msgs/msg/Image` | 32FC1 depth map (depth in meters) |
| `/sc/depth/camera_info` | `sensor_msgs/msg/CameraInfo` | Depth camera intrinsics |
| `/sc/depth_aligned/image` | `sensor_msgs/msg/Image` | 32FC1 depth registered onto the visible image plane |
| `/sc/depth_aligned/camera_info` | `sensor_msgs/msg/CameraInfo` | Aligned depth camera intrinsics |
| `/sc/depth/points` | `sensor_msgs/msg/PointCloud2` | XYZ point cloud in depth frame |
| `/sc/rgbd/points` | `sensor_msgs/msg/PointCloud2` | Colored XYZRGB point cloud |
| `/sc/depth/infrared_left` | `sensor_msgs/msg/Image` | Left infrared sensor feed (`mono8` or `16UC1`) |
| `/sc/depth/infrared_right` | `sensor_msgs/msg/Image` | Right infrared sensor feed (`mono8` or `16UC1`) |
| `/sc/imu/data_raw` | `sensor_msgs/msg/Imu` | Accel ($m/s^2$) and Gyro ($rad/s$) stream |
| `/tf` & `/tf_static` | `tf2_msgs/msg/TFMessage` | Dynamic `color_to_depth` and static optical frames |

---

## 6. Configuration Parameters

All parameters can be tuned in [`cfg/sc_config.yaml`](cfg/sc_config.yaml) or passed on the command line:

```yaml
visible_enable: true                     # Enable color camera
visible_apply_gamma_correction: false
visible_framerate: 30.0
depth_enable: true                       # Enable depth stream
depth_aligned_enable: true               # Enable RGB-aligned depth map
depth_pcloud_enable: false               # Raw depth XYZ point cloud
rgbd_enable: true                        # Colored RGB-D point cloud
depth_framerate: 30.0
depth_resolution: "Default"              # "Default", "SXGA", "VGA", "QVGA"
infrared_enable: false                   # Enable IR camera stream
infrared_mode: "Default"                 # "Default", "BothCameras", "LeftCameraOnly", "RightCameraOnly"
imu_enable: false                        # Enable IMU publishing
imu_update_rate: "Default"               # "Default", "AccelAndGyro_1000Hz", "AccelAndGyro_400Hz", ...
initial_projector_power: 1.0             # IR laser projector power (0.0 to 1.0)
latency_reducer_enabled: true
sensor_initialization_timeout: 20000     # Sensor connection timeout (ms)
```

Runtime dynamic parameters can be modified on the fly:
```bash
# Example: adjust exposure or gain
ros2 param set /sc/sc_node visible_initial_exposure 0.025
ros2 param set /sc/sc_node visible_initial_gain 4.0
```
