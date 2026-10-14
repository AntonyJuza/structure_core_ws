# Structure Core ROS 2 Jazzy Workspace

ROS 2 workspace containing the modernized, high-performance **`structure_core_ros2_driver`** package for the **Occipital Structure Core** depth and infrared camera, upgraded and verified for **ROS 2 Jazzy Jalisco on Ubuntu 24.04 LTS (Noble)**.

---

## 🚀 Quick Start

### 1. One-Time USB Permission Setup (Required)
The Structure Core communicates over userspace USB through `libusb` and requires non-root read/write permissions.

Install the udev rule once:
```bash
sudo ./src/structure_core_ros2_driver/scripts/install_udev_rules.sh
sudo udevadm control --reload-rules && sudo udevadm trigger
```

*(Temporary alternative: `sudo chmod 666 /dev/bus/usb/$(lsusb -d 277a: | awk '{print $2"/"$4}' | sed 's/://')`)*

---

### 2. Build the Workspace

```bash
source /opt/ros/jazzy/setup.bash
colcon build --symlink-install --packages-select structure_core_ros2_driver
source install/setup.bash
```

---

### 3. Launch the Driver

#### Option A: Camera Connected Locally (PC)
```bash
# 1. Launch camera driver
./start_structure_core.sh

# 2. Or launch camera driver + RViz2 visualization together
./start_structure_core.sh --rviz
```

#### Option B: Camera Connected to Remote Raspberry Pi
When the Structure Core is plugged into a **Raspberry Pi** and you want to visualize streams on your **PC**:

1. **On the Raspberry Pi (headless, low overhead):**
   ```bash
   # In depth_ws on the Pi:
   source /opt/ros/jazzy/setup.bash
   source install/setup.bash
   ros2 launch structure_core_ros2_driver sc.launch.py
   ```

2. **On your PC (RViz visualization):**
   ```bash
   # Convenience script:
   ./view_structure_core.sh

   # Or via ROS 2 launch:
   source /opt/ros/jazzy/setup.bash
   source install/setup.bash
   ros2 launch structure_core_ros2_driver sc_rviz2.launch.py
   ```
   *(Ensure both Pi and PC are on the same network and have matching `ROS_DOMAIN_ID`)*

---


## 📡 Published Topics

| Topic | Type | Resolution / Format | Description |
|---|---|---|---|
| `/sc/rgb/image` | `sensor_msgs/msg/Image` | 1280×960 / 640×480, `rgb8` / `mono8` | Visible color feed |
| `/sc/rgb/camera_info` | `sensor_msgs/msg/CameraInfo` | - | Visible camera intrinsics |
| `/sc/depth/image` | `sensor_msgs/msg/Image` | 640×480 / 320×240, `32FC1` | Rectified depth map (values in **meters**) |
| `/sc/depth/camera_info` | `sensor_msgs/msg/CameraInfo` | - | Depth camera intrinsics |
| `/sc/depth_aligned/image` | `sensor_msgs/msg/Image` | Matched to RGB, `32FC1` | Depth registered directly onto color image plane |
| `/sc/depth_aligned/camera_info` | `sensor_msgs/msg/CameraInfo` | - | Aligned depth intrinsics |
| `/sc/rgbd/points` | `sensor_msgs/msg/PointCloud2` | XYZRGB 3D PointCloud | 3D colored point cloud |
| `/sc/depth/points` | `sensor_msgs/msg/PointCloud2` | XYZ 3D PointCloud | Raw depth 3D point cloud (when enabled) |
| `/sc/depth/infrared_left` | `sensor_msgs/msg/Image` | 640×480 / 320×240, `mono8` / `16UC1` | Left IR stream (when enabled) |
| `/sc/depth/infrared_right` | `sensor_msgs/msg/Image` | 640×480 / 320×240, `mono8` / `16UC1` | Right IR stream (when enabled) |
| `/sc/imu/data_raw` | `sensor_msgs/msg/Imu` | Accel ($m/s^2$) + Gyro ($rad/s$) | High-frequency synchronized IMU stream |
| `/tf` & `/tf_static` | `tf2_msgs/msg/TFMessage` | - | Dynamic `color_to_depth` & static optical frames |

---

## 📐 TF Frame Hierarchy

The driver publishes TF transforms compliant with ROS REP-103 and REP-105:
```
sc_map
├── sc_color_frame (optical frame: Z forward, X right, Y down)
└── sc_cloud_tf_frame (body frame: X forward, Y left, Z up)
```
When depth streaming is active, dynamic transform `sc_depth_frame -> sc_color_frame` is published automatically based on the sensor's factory calibration matrix.

---

## ⚙️ Configuration & Parameters

All node settings are configured via [`src/structure_core_ros2_driver/cfg/sc_config.yaml`](src/structure_core_ros2_driver/cfg/sc_config.yaml):

| Parameter | Type | Default | Description |
|---|---|---|---|
| `visible_enable` | `bool` | `true` | Enable color / visible camera stream |
| `visible_framerate` | `double` | `30.0` | Target framerate for visible camera |
| `depth_enable` | `bool` | `true` | Enable depth camera stream |
| `depth_aligned_enable`| `bool` | `true` | Enable RGB-aligned depth map |
| `rgbd_enable` | `bool` | `true` | Enable colored 3D point cloud (`/sc/rgbd/points`) |
| `depth_pcloud_enable` | `bool` | `false` | Enable raw depth point cloud (`/sc/depth/points`) |
| `depth_framerate` | `double` | `30.0` | Target framerate for depth stream |
| `depth_resolution` | `string` | `"Default"` | Resolution: `"Default"`, `"SXGA"`, `"VGA"`, `"QVGA"` |
| `infrared_enable` | `bool` | `false` | Enable infrared stream |
| `infrared_mode` | `string` | `"Default"` | Mode: `"BothCameras"`, `"LeftCameraOnly"`, `"RightCameraOnly"` |
| `imu_enable` | `bool` | `false` | Enable IMU streaming |
| `imu_update_rate` | `string` | `"Default"` | IMU rate: `"AccelAndGyro_1000Hz"`, `"400Hz"`, `"200Hz"`, `"100Hz"` |
| `initial_projector_power`| `double` | `1.0` | IR laser projector power (`0.0` to `1.0`) |
| `depth_range_mode` | `int` | `1` | Depth range: `0` (VeryShort), `1` (Short), `2` (Medium), `3` (Long), `4` (VeryLong), `5` (Hybrid) |
| `visible_initial_exposure`| `double` | `0.016` | Initial visible exposure in seconds (0.0 to 0.03) |
| `visible_initial_gain` | `double` | `2.0` | Initial visible gain (1.0 to 8.0) |

### On-the-Fly Dynamic Parameter Tuning
Dynamic parameters can be modified at runtime without restarting the driver:
```bash
# Adjust exposure (in seconds):
ros2 param set /sc/sc_node visible_initial_exposure 0.02

# Adjust gain:
ros2 param set /sc/sc_node visible_initial_gain 4.0

# Switch depth range preset:
ros2 param set /sc/sc_node depth_range_mode 2
```

---

## 🛠️ Key Improvements in the Jazzy Driver

1. **Jazzy API Compliance**:
   - Replaced old Crystal-era node constructor with standard `rclcpp::NodeOptions`.
   - Uses `add_on_set_parameters_callback` and `declare_or_get` pattern to prevent parameter declaration conflicts.
2. **Fixed Static Transforms in Jazzy**:
   - Migrated launch files to the new `static_transform_publisher` required argument format (`--frame-id` and `--child-frame-id`).
3. **Correct REP-103 IMU Units**:
   - Raw accelerometer readings from the hardware ($g$) are converted to SI units ($m/s^2$, $1g = 9.80665 m/s^2$) and published to standard topic `/sc/imu/data_raw`.
4. **Self-Contained Structure SDK**:
   - Bundles `libStructure.so` and headers directly under [`src/structure_core_ros2_driver/structure_sdk/`](src/structure_core_ros2_driver/structure_sdk) with automatic installation and RPATH configuration. No manual library paths needed.
5. **Fast Zero-Copy Data Processing**:
   - Eliminated stack allocations (which previously allocated up to 5 MB on the thread stack) and slow vector reallocations in image conversion callbacks.

---

## 🔍 Troubleshooting

- **`No MAGIC from USB interface 0 (error=7)` or `Timeout` during boot:**
  - This indicates `libusb` lacks read/write permissions for the USB device. Run `sudo ./src/structure_core_ros2_driver/scripts/install_udev_rules.sh` and re-plug the camera.
- **Topics not showing up in RViz2:**
  - Ensure the Fixed Frame in RViz2 is set to `sc_map` or `sc_color_frame`.
- **RMW Discovery across subnets:**
  - If topics are not seen by external machines on your network, ensure `export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp` or `export ROS_AUTOMATIC_DISCOVERY_RANGE=SUBNET` matches across machines.
