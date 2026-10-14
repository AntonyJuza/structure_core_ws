#!/usr/bin/env bash
set -e

WORKSPACE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source ROS 2 Jazzy environment
if [ -f "/opt/ros/jazzy/setup.bash" ]; then
    source /opt/ros/jazzy/setup.bash
else
    echo "[ERROR] /opt/ros/jazzy/setup.bash not found. Is ROS 2 Jazzy installed?" >&2
    exit 1
fi

# Source workspace install
if [ -f "${WORKSPACE_DIR}/install/setup.bash" ]; then
    source "${WORKSPACE_DIR}/install/setup.bash"
else
    echo "[WARNING] ${WORKSPACE_DIR}/install/setup.bash not found. Building workspace first..."
    cd "${WORKSPACE_DIR}"
    colcon build --symlink-install --packages-select structure_core_ros2_driver
    source "${WORKSPACE_DIR}/install/setup.bash"
fi

# Check USB device detection and permissions
USB_DEV=$(lsusb -d 277a: 2>/dev/null || true)
if [ -n "${USB_DEV}" ]; then
    BUS=$(echo "${USB_DEV}" | awk '{print $2}')
    DEV=$(echo "${USB_DEV}" | awk '{print $4}' | sed 's/://')
    DEV_PATH="/dev/bus/usb/${BUS}/${DEV}"
    if [ -e "${DEV_PATH}" ] && [ ! -w "${DEV_PATH}" ]; then
        echo "================================================================================"
        echo "[WARNING] Structure Core USB camera detected at ${DEV_PATH},"
        echo "          but current user does not have write access to it."
        echo "          To fix this, install udev rules once using:"
        echo ""
        echo "          sudo ${WORKSPACE_DIR}/src/structure_core_ros2_driver/scripts/install_udev_rules.sh"
        echo "          sudo udevadm control --reload-rules && sudo udevadm trigger"
        echo ""
        echo "          Or temporarily grant permissions:"
        echo "          sudo chmod 666 ${DEV_PATH}"
        echo "================================================================================"
    fi
else
    echo "[INFO] No Structure Core USB device currently detected via lsusb (VID 277a)."
fi

# Parse arguments
LAUNCH_ARGS=()
for arg in "$@"; do
    case "$arg" in
        --rviz)
            LAUNCH_ARGS+=("use_rviz:=true")
            ;;
        -h|--help)
            echo "Usage: $0 [--rviz] [extra launch arguments...]"
            echo "Example: $0 --rviz"
            echo "Example: $0 use_rviz:=true visible_framerate:=15.0"
            exit 0
            ;;
        *)
            LAUNCH_ARGS+=("$arg")
            ;;
    esac
done

echo "[INFO] Starting Structure Core ROS 2 Jazzy Driver..."
exec ros2 launch structure_core_ros2_driver sc.launch.py "${LAUNCH_ARGS[@]}"
