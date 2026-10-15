#!/usr/bin/env bash
# ==============================================================================
# Occipital Structure Core Depth Camera ROS 2 Jazzy Driver Startup Script
# ==============================================================================
set -e

# 1. Parse quick arguments
LAUNCH_ARGS=("publish_standalone_tf:=false")
for arg in "$@"; do
    case "$arg" in
        --rviz)
            LAUNCH_ARGS+=("use_rviz:=true")
            ;;
        --standalone)
            LAUNCH_ARGS+=("publish_standalone_tf:=true")
            ;;
        -h|--help)
            echo "Usage: $0 [--rviz] [--standalone] [extra launch arguments...]"
            echo "Example: $0"
            echo "Example: $0 --rviz"
            exit 0
            ;;
        *)
            LAUNCH_ARGS+=("$arg")
            ;;
    esac
done

# 2. Source ROS 2 Jazzy environment
if [ -f "$HOME/ros2_jazzy/install/setup.bash" ]; then
    source "$HOME/ros2_jazzy/install/setup.bash"
elif [ -f "$HOME/setup_env.sh" ]; then
    source "$HOME/setup_env.sh"
elif [ -f "/opt/ros/jazzy/setup.bash" ]; then
    source /opt/ros/jazzy/setup.bash
fi

# CycloneDDS configuration
export RMW_IMPLEMENTATION=rmw_cyclonedds_cpp
if [ -f "$HOME/cyclonedds.xml" ]; then
    export CYCLONEDDS_URI=file://"$HOME/cyclonedds.xml"
fi

# 3. Source Structure Core depth workspace
if [ -f "$HOME/depth_ws/install/setup.bash" ]; then
    source "$HOME/depth_ws/install/setup.bash"
elif [ -f "$HOME/depth_ws/structure_core_ws/install/setup.bash" ]; then
    source "$HOME/depth_ws/structure_core_ws/install/setup.bash"
fi

# 4. Check USB device detection and permissions
USB_DEV=$(lsusb -d 277a: 2>/dev/null || true)
if [ -n "${USB_DEV}" ]; then
    BUS=$(echo "${USB_DEV}" | awk '{print $2}')
    DEV=$(echo "${USB_DEV}" | awk '{print $4}' | sed 's/://')
    DEV_PATH="/dev/bus/usb/${BUS}/${DEV}"
    if [ -e "${DEV_PATH}" ] && [ ! -w "${DEV_PATH}" ]; then
        echo "================================================================================"
        echo "[WARNING] Structure Core USB camera detected at ${DEV_PATH},"
        echo "          granting write access for userspace streaming..."
        sudo chmod 666 "${DEV_PATH}" 2>/dev/null || true
        echo "================================================================================"
    fi
else
    echo "[WARNING] No Structure Core USB device currently detected via lsusb (VID 277a)."
fi

echo "============================================="
echo " Starting Structure Core ROS 2 Driver"
echo " Camera Model  : Occipital Structure Core (USB 3.0)"
echo " Distro        : ${ROS_DISTRO:-jazzy}"
echo " Obstacle Cloud: /sc/depth/points (XYZ) & /sc/rgbd/points (XYZRGB)"
echo " Color Stream  : /sc/rgb/image"
echo " Depth Stream  : /sc/depth/image"
echo " IMU Stream    : /sc/imu/data_raw"
echo "============================================="

exec ros2 launch structure_core_ros2_driver sc.launch.py "${LAUNCH_ARGS[@]}"
