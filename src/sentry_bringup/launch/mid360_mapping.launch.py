"""Host-native MID360 mapping launch.

This is the host replacement for the former Docker mapping flow.  It starts the
pb2025 real-car SLAM backend with the verified real-car static TFs and keeps all
mapping-related nodes as independent processes for easier tuning/debugging.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _without_conda_libs(value):
    return ":".join(
        path
        for path in value.split(":")
        if path and "anaconda" not in path and "conda" not in path
    )


def generate_launch_description():
    pb_bringup_pkg = get_package_share_directory("pb2025_nav_bringup")

    params_file = LaunchConfiguration("params_file")
    use_rviz = LaunchConfiguration("use_rviz")
    use_composition = LaunchConfiguration("use_composition")

    pb2025_mapping = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pb_bringup_pkg, "launch", "rm_navigation_reality_launch.py")
        ),
        launch_arguments={
            "slam": "True",
            "use_robot_state_pub": "False",
            "use_rviz": use_rviz,
            "use_composition": use_composition,
            "use_respawn": "False",
            "params_file": params_file,
        }.items(),
    )

    front_mid360_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="base_to_front_mid360_tf",
        arguments=[
            "--x", "-0.001",
            "--y", "0.17185",
            "--z", "0.200",
            "--roll", "0.7854",
            "--pitch", "0.0",
            "--yaw", "0.0",
            "--frame-id", "base_footprint",
            "--child-frame-id", "front_mid360",
        ],
        output="screen",
    )

    gimbal_yaw_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="base_to_gimbal_yaw_tf",
        arguments=[
            "--x", "0",
            "--y", "0",
            "--z", "0",
            "--roll", "0",
            "--pitch", "0",
            "--yaw", "0",
            "--frame-id", "base_footprint",
            "--child-frame-id", "gimbal_yaw",
        ],
        output="screen",
    )

    gimbal_yaw_fake_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="gimbal_yaw_to_fake_tf",
        arguments=[
            "--x", "0",
            "--y", "0",
            "--z", "0",
            "--roll", "0",
            "--pitch", "0",
            "--yaw", "0",
            "--frame-id", "gimbal_yaw",
            "--child-frame-id", "gimbal_yaw_fake",
        ],
        output="screen",
    )

    return LaunchDescription(
        [
            SetEnvironmentVariable("FASTDDS_BUILTIN_TRANSPORTS", "UDPv4"),
            SetEnvironmentVariable("RMW_FASTRTPS_USE_SHM", "0"),
            SetEnvironmentVariable(
                "LD_LIBRARY_PATH",
                _without_conda_libs(os.environ.get("LD_LIBRARY_PATH", "")),
            ),
            DeclareLaunchArgument(
                "params_file",
                default_value=os.path.join(
                    pb_bringup_pkg, "config", "reality", "nav2_params.yaml"
                ),
                description="pb2025 real-car mapping parameters",
            ),
            DeclareLaunchArgument(
                "use_rviz",
                default_value="True",
                description="Start RViz with the pb2025 mapping view",
            ),
            DeclareLaunchArgument(
                "use_composition",
                default_value="False",
                description="Use independent processes by default for easier debugging",
            ),
            front_mid360_tf,
            gimbal_yaw_tf,
            gimbal_yaw_fake_tf,
            pb2025_mapping,
        ]
    )
