import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():

    camera_model_1 = 'sc' 

    sc_config = os.path.join(
        get_package_share_directory( 'structure_core_ros2_driver' ), 
        'cfg', camera_model_1+'_config.yaml'
    )

    rviz2_config = os.path.join(get_package_share_directory('structure_core_ros2_driver'), 'rviz', 'sc_ros2_config.rviz')

    os.environ['RCUTILS_CONSOLE_OUTPUT_FORMAT'] = '{time}: [{name}] [{severity}] - {message}'

    return LaunchDescription( [
        Node(
            node_namespace=camera_model_1,             # must match the namespace in config -> YAML
            node_name=camera_model_1 + '_node',        # must match the node name in config -> YAML
            package='structure_core_ros2_driver',
            node_executable='sc_wrapper_node',
            output='screen',
            parameters=[
                sc_config,  # sc parameters
            ],
        ),

        Node(package='tf2_ros',
             node_executable='static_transform_publisher',
             node_name='tf_broadcaster',
             arguments=['0', '0', '0',  '0', '0', '0', camera_model_1 + '_map', camera_model_1 +'_color_frame']),

         Node(package='tf2_ros',
             node_executable='static_transform_publisher',
             node_name='depth_tf_broadcaster',
             arguments=['0', '0', '0',  '0', '0', '0', camera_model_1 + '_map', camera_model_1 + '_cloud_tf_frame']),

        Node(package='rviz2',
             node_executable='rviz2',
             node_name='rviz2',
             output = 'screen',
             arguments=['-d', rviz2_config])

])
