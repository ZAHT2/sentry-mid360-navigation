"""Cartographer 3D→2D 建图启动"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    bringup_pkg = get_package_share_directory("sentry_bringup")
    cartographer_config_dir = os.path.join(bringup_pkg, "config")
    configuration_basename = "cartographer.lua"

    use_sim_time = LaunchConfiguration("use_sim_time", default="false")

    cartographer_node = Node(
        package="cartographer_ros",
        executable="cartographer_node",
        name="cartographer_node",
        output="screen",
        parameters=[{"use_sim_time": use_sim_time}],
        arguments=[
            "-configuration_directory", cartographer_config_dir,
            "-configuration_basename", configuration_basename,
        ],
        remappings=[
            ("points2", "/livox/lidar"),   # Mid360 PointCloud2 话题
            ("odom", "/odom"),
        ],
    )

    occupancy_grid_node = Node(
        package="cartographer_ros",
        executable="cartographer_occupancy_grid_node",
        name="cartographer_occupancy_grid_node",
        output="screen",
        parameters=[
            {"use_sim_time": use_sim_time},
            {"resolution": 0.05},
        ],
    )

    return LaunchDescription([
        DeclareLaunchArgument("use_sim_time", default_value="false"),
        cartographer_node,
        occupancy_grid_node,
    ])
