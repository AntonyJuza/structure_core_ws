# Topics and Parameters {#ros_main_features}

<!-- more -->

# Published Topics {#published_topics}

The full list of topics can be listed during program execution by running `rostopic list`.

Topic                           | Description
--------------------------------|-------------
sc/color_to_depth_tf            | Transformation matrix to bring the visible (mono or RGB) camera frame into the depth frame of reference
sc/depth/camera_info            | Depth frame intrinsics for the current frame
sc/depth/image                  | Depth image (float depth values in meters)
sc/imu/imu_msg                  | 3-axis accelerometer data (m/s/s) and 3-axis gyroscope data in Euler angles (rad/sec)
sc/infrared_left/camera_info    | Left infrared intrinsics for the current frame
sc/infrared_left/image          | Left infrared image (16bit data)
sc/infrared_right/camera_info    | Right infrared intrinsics for the current frame
sc/infrared_right/image         | Right infrared image (16bit data)
sc/rgb/camera_info              | Visible (mono or RGB) camera frame intrinsics for the current frame
sc/rgb/image                    | Visible camera image. Either mono or RGB, depending on the Structure Core model
sc/rgbd/points                  | RGB-D point cloud

# Structure Core Static Parameters {#sc_static_params}

Static parameters can be modified before launch by modifying the **launch/sc.launch** file for ROS-only runs, or the **launch/sc_rviz.launch** file for ROS+Rviz runs.

Static Parameter                        | Type  | Description | Default Value 
----------------------------------------|-------|-------------|-------------- 
demosaic_method                         | {`EdgeAware`, `Bilinear`} | The demosaicking method to use for color-enabled Structure Core visible frames. Will do nothing for monochrome visible frames | `EdgeAware`
depth_apply_correction_before_stream    | boolean | Set to true to apply a correction and clean filter to the depth before streaming. Depth frames received from the driver are post-processed before arrival.  | false
depth_enable                            | boolean | Set to true to enable depth streaming | false
depth_framerate                         | integer | The target framerate for the depth sensor | 30
depth_resolution                        | {`640x480`, `320x240`, `1280x960`} | The target resolution for streamed depth frames | `640x480`
frame_sync_enabled                      | boolean | Set to true to enable frame synchronization between the visible and depth streams | false
imu_enable                              | boolean | Set to true to enable accelerometer and gyroscope streaming | false
imu_update_rate                         | {`AccelAndGyro_800Hz`, `AccelAndGyro_100Hz`, `AccelAndGyro_200Hz`, `AccelAndGyro_1000Hz`} | the target streaming rate for IMU data | `AccelAndGyro_800Hz`
infrared_disable_intensity_balance      | boolean | Setting this to true will eliminate saturation issues, but might result in sparser depth | false
infrared_enable                         | boolean | Set to true to enable infrared streaming | false
infrared_framerate                      | integer | The target framerate for the infrared camera| 30
infrared_mode                           | {`BothCameras`, `LeftCameraOnly`, `RightCameraOnly`} | Specifies how to stream the infrared frames | `BothCameras`
infrared_resolution                     | {`1280x960`} | The target resolution at which to stream depth frames | `1280x960`
initial_projector_power                 | float (0.0 - 1.0] | Laser projector power setting from 0.0 to 1.0 inclusive. Projector will only activate if required by streaming configuration | 1.0
latency_reducer_enabled                 | boolean | Setting this to true will reduce latency. Frame dropping might increase | false 
low_latency_imu                         | boolean | Set to true to deliver IMU events on a separate, dedicated background thread | false
rgbd_enable                             | boolean | Set to true to enable RGB-D streaming | false
rviz_frame                              | boolean | Set to true to change point cloud reference frame to rviz frame | false
sensor_initialization_timeout           | integer | Maximum amount of time (in milliseconds) to wait for a sensor to connect before throwing a timeout error | 6000
sensor_serial                           | string | Serial number of the sensor to stream. If null, the first connected sensor will be used | null
visible_apply_gamma_correction          | boolean | Set to true to apply gamma correction to incoming visible frames | false
visible_enable                          | boolean | Set to true to enable visible streaming | false
visible_framerate                       | integer | The target framerate for the visible camera | 30
visible_resolution                      | {`640x480`} | The target resolution for streamed visible frames | `640x480`

# Structure Core Dynamic Parameters {#sc_dynamic_params}

The following parameters are dynamic and can be changed during program execution, without restarting the Structure Core ROS driver. They can be accessed by using the `/sc/sc_node/` prefix, e.g. `/sc/sc_node/visible_initial_gain`.

Dynamic Parameter                           | Type  | Description | Default Value 
--------------------------------------------|-------|-------------|-------------- 
depth_apply_correction                      | boolean | Applies a correction and clean filter to the depth on-the-fly, modifying the current depth frame in place. Note that if `depth_apply_correction_before_stream` is true, this setting does nothing, as the correction has already been applied in the driver before frame arrival | false
depth_range_mode                            | {`VeryShort`, `Short`, `Medium`, `Long`, `VeryLong`, `Hybrid`, `BodyScanning`} | The depth range mode for streamed depth frames. Modifies the min/max range of the depth values.<br> - VeryShort: 0.35m - 0.92m <br> - Short: 0.41m - 1.36m<br> - Medium: 0.52m - 5.23m<br>  - Long: 0.58m - 8.0m<br>  - VeryLong: 0.58m - 10.0m<br>  - Hybrid: 0.35m - 10.0m<br>  - BodyScanning: Specific configuration for scanning bodies. | `Short`
dynamic_calibration_mode                    | {`Off`, `OneShotPersistent`, `ContinuousNonPersistent`} | The dynamic calibration mode to use during depth streaming.<br> - OneShotPersistent: a single dynamic calibration cycle is performed when depth streaming starts. <br> - ContinuousNonPersistent: dynamic calibration is performed continuously as the sensor streams depth data | `Off`
infrared_auto_exposure_enabled              | boolean | Enable auto-exposure for infrared frames | false
infrared_initial_exposure                   | float [0.0 - 0.03] | The initial exposure on the infrared cameras | 0.0146
infrared_initial_gain                       | integer [0 - 3] | The initial gain on the infrared cameras | 3
visible_initial_exposure                    | float [0.0 - 0.03] | The initial exposure on the visible camera, in seconds | 0.016
visible_initial_gain                        | integer [1 - 8] | The initial gain on the visible camera | 2.0

<br>

## Modifying Parameters (ROS) {#mod_dynamic_params_ros}

Use the `rqt_reconfigure` ROS package to change dynamic parameters: 

~~~{.sh}

rosrun rqt_reconfigure rqt_reconfigure

~~~

The **rqt_reconfigure** window looks like this:

![Caption text](rqt_reconfigure.png)

<br>

## Modifying Parameters (ROS2) {#mod_dynamic_params_ros2}

The **ros2 param** command line tool allows users to change dynamic parameters. 

- Use `ros2 param list` to list all static and dynamic parameters for Structure Core. 
- Set a dynamic parameter using the `set` command, e.g.

~~~{.sh}

ros2 param set /sc/sc_node/<dynamic parameter> <value>

~~~
