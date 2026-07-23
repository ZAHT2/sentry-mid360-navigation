"""Nav2 导航栈启动（需已有地图）"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    nav_pkg    = get_package_share_directory("sentry_navigation")
    nav2_pkg   = get_package_share_directory("nav2_bringup")

    nav2_params = os.path.join(nav_pkg, "config", "nav2_params.yaml")

    map_yaml = LaunchConfiguration("map")
    use_sim_time = LaunchConfiguration("use_sim_time", default="false")

    return LaunchDescription([
        DeclareLaunchArgument("map", description="Full path to map yaml file"),
        DeclareLaunchArgument("use_sim_time", default_value="false"),

        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(nav2_pkg, "launch", "bringup_launch.py")
            ),
            launch_arguments={
                "map": map_yaml,
                "use_sim_time": use_sim_time,
                "params_file": nav2_params,
            }.items(),
        ),
    ])
