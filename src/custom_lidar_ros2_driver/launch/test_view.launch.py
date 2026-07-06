import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    pkg_share = get_package_share_directory('custom_lidar_ros2_driver')
    params_file = os.path.join(pkg_share, 'params', 'lidar.yaml')

    return LaunchDescription([
        Node(
            package='custom_lidar_ros2_driver',
            executable='custom_lidar_ros2_driver_node',
            name='custom_lidar_ros2_driver',
            output='screen',
            parameters=[params_file],
        ),

        # Chỉ dùng khi test LiDAR độc lập trong RViz.
        Node(
            package='tf2_ros',
            executable='static_transform_publisher',
            name='world_to_laser_tf',
            arguments=['0', '0', '0', '0', '0', '0', 'world', 'laser_frame'],
        ),
    ])
