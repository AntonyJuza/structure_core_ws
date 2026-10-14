import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

def generate_launch_description():
    pkg_share = get_package_share_directory('structure_core_ros2_driver')

    default_config = os.path.join(pkg_share, 'cfg', 'sc_config.yaml')
    default_rviz = os.path.join(pkg_share, 'rviz', 'sc_ros2_config.rviz')

    name_arg = DeclareLaunchArgument(
        'name',
        default_value='sc',
        description='Camera name and ROS namespace'
    )

    config_arg = DeclareLaunchArgument(
        'config_file',
        default_value=default_config,
        description='Path to YAML configuration file'
    )

    use_rviz_arg = DeclareLaunchArgument(
        'use_rviz',
        default_value='false',
        description='Whether to start RViz2'
    )

    rviz_config_arg = DeclareLaunchArgument(
        'rviz_config',
        default_value=default_rviz,
        description='Path to RViz configuration file'
    )

    name = LaunchConfiguration('name')
    config_file = LaunchConfiguration('config_file')
    use_rviz = LaunchConfiguration('use_rviz')
    rviz_config = LaunchConfiguration('rviz_config')

    camera_node = Node(
        package='structure_core_ros2_driver',
        executable='sc_wrapper_node',
        name='sc_node',
        namespace=name,
        output='screen',
        parameters=[config_file, {'name': name}],
    )

    # Static transforms compliant with ROS 2 Jazzy static_transform_publisher
    tf_color_broadcaster = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='color_tf_broadcaster',
        arguments=[
            '--frame-id', [name, '_map'],
            '--child-frame-id', [name, '_color_frame'],
            '--x', '0', '--y', '0', '--z', '0',
            '--roll', '0', '--pitch', '0', '--yaw', '0'
        ]
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
        ]
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', rviz_config],
        condition=IfCondition(use_rviz),
        output='screen'
    )

    return LaunchDescription([
        name_arg,
        config_arg,
        use_rviz_arg,
        rviz_config_arg,
        camera_node,
        tf_color_broadcaster,
        tf_depth_broadcaster,
        rviz_node
    ])
