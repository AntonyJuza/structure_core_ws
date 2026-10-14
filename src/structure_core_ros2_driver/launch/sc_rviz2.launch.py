import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('structure_core_ros2_driver')
    default_rviz = os.path.join(pkg_share, 'rviz', 'sc_ros2_config.rviz')

    name_arg = DeclareLaunchArgument(
        'name',
        default_value='sc',
        description='Camera name and ROS namespace'
    )

    rviz_config_arg = DeclareLaunchArgument(
        'rviz_config',
        default_value=default_rviz,
        description='Path to RViz configuration file'
    )

    publish_tf_arg = DeclareLaunchArgument(
        'publish_tf',
        default_value='false',
        description='Publish static TF transforms locally on this viewer (set to true if remote Pi does not broadcast static TFs)'
    )

    name = LaunchConfiguration('name')
    rviz_config = LaunchConfiguration('rviz_config')
    publish_tf = LaunchConfiguration('publish_tf')

    # Optional static transforms compliant with ROS 2 Jazzy static_transform_publisher
    tf_color_broadcaster = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='color_tf_broadcaster',
        arguments=[
            '--frame-id', [name, '_map'],
            '--child-frame-id', [name, '_color_frame'],
            '--x', '0', '--y', '0', '--z', '0',
            '--roll', '0', '--pitch', '0', '--yaw', '0'
        ],
        condition=IfCondition(publish_tf)
    )

    tf_depth_broadcaster = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='depth_tf_broadcaster',
        arguments=[
            '--frame-id', [name, '_map'],
            '--child-frame-id', [name, '_cloud_tf_frame'],
            '--x', '0', '--y', '0', '--z', '0',
            '--roll', '0', '--pitch', '0', '--yaw', '0'
        ],
        condition=IfCondition(publish_tf)
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2_viewer',
        arguments=['-d', rviz_config],
        output='screen'
    )

    return LaunchDescription([
        name_arg,
        rviz_config_arg,
        publish_tf_arg,
        tf_color_broadcaster,
        tf_depth_broadcaster,
        rviz_node
    ])
