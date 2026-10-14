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

# Help message
if [[ "$*" == *"--help"* ]] || [[ "$*" == *"-h"* ]]; then
    echo "Usage: ./view_structure_core.sh [--tf] [extra launch arguments...]"
    echo ""
    echo "Options:"
    echo "  --tf       Publish static TF transforms locally on this PC (if not broadcasted by Pi)"
    echo "  -h, --help Show this help message"
    echo ""
    echo "Examples:"
    echo "  ./view_structure_core.sh"
    echo "  ./view_structure_core.sh --tf"
    echo "  ./view_structure_core.sh publish_tf:=true"
    exit 0
fi

LAUNCH_ARGS=()
for arg in "$@"; do
    case "$arg" in
        --tf)
            LAUNCH_ARGS+=("publish_tf:=true")
            ;;
        *)
            LAUNCH_ARGS+=("$arg")
            ;;
    esac
done

echo "[INFO] Starting RViz2 viewer for remote Structure Core camera..."
exec ros2 launch structure_core_ros2_driver sc_rviz2.launch.py "${LAUNCH_ARGS[@]}"
