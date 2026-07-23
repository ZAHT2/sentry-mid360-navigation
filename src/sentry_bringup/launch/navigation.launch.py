"""Nav2 导航启动（加载已有地图）"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    bringup_pkg = get_package_share_directory("sentry_bringup")
    nav2_pkg = get_package_share_directory("nav2_bringup")
    nav_params = os.path.join(
        get_package_share_directory("sentry_navigation"), "config", "nav2_params.yaml"
    )

    map_yaml = LaunchConfiguration("map")
    use_sim_time = LaunchConfiguration("use_sim_time", default="false")

    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_pkg, "launch", "bringup_launch.py")
        ),
        launch_arguments={
            "map": map_yaml,
            "use_sim_time": use_sim_time,
            "params_file": nav_params,
        }.items(),
    )

    return LaunchDescription([
        DeclareLaunchArgument("map", description="Full path to map yaml file"),
        DeclareLaunchArgument("use_sim_time", default_value="false"),
        nav2_launch,
    ])
