"""Host-native real-car navigation for MID360 map11.

This launch replaces the former Docker/host split graph.  It starts the
pb2025 navigation backend, the real-car static TFs, and the verified chassis
serial bridge in one host ROS graph.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


WORKSPACE_ROOT = "/home/zaht/sentry_nav_ws"


def _without_conda_libs(value):
    return ":".join(
        path
        for path in value.split(":")
        if path and "anaconda" not in path and "conda" not in path
    )


def generate_launch_description():
    pb_bringup_pkg = get_package_share_directory("pb2025_nav_bringup")
    sentry_bringup_pkg = get_package_share_directory("sentry_bringup")

    map_file = LaunchConfiguration("map")
    prior_pcd_file = LaunchConfiguration("prior_pcd_file")
    params_file = LaunchConfiguration("params_file")
    use_rviz = LaunchConfiguration("use_rviz")
    use_composition = LaunchConfiguration("use_composition")
    serial_port = LaunchConfiguration("serial_port")
    chassis_params_file = LaunchConfiguration("chassis_params_file")

    pb2025_navigation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pb_bringup_pkg, "launch", "rm_navigation_reality_launch.py")
        ),
        launch_arguments={
            "slam": "False",
            "use_robot_state_pub": "False",
            "use_rviz": use_rviz,
            "use_composition": use_composition,
            "use_respawn": "False",
            "map": map_file,
            "prior_pcd_file": prior_pcd_file,
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

    base_link_tf = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="base_footprint_to_base_link_tf",
        arguments=[
            "--x", "0",
            "--y", "0",
            "--z", "0",
            "--roll", "0",
            "--pitch", "0",
            "--yaw", "0",
            "--frame-id", "base_footprint",
            "--child-frame-id", "base_link",
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

    chassis_bridge = Node(
        package="sentry_hardware",
        executable="chassis_bridge_node",
        name="chassis_bridge_node",
        output="screen",
        additional_env={
            "LD_LIBRARY_PATH": _without_conda_libs(
                os.environ.get("LD_LIBRARY_PATH", "")
            ),
        },
        parameters=[
            chassis_params_file,
            {
                "serial_port": serial_port,
                "cmd_vel_topic": "/cmd_vel",
                "publish_tf": False,
                "odom_topic": "/chassis/odom_raw",
                "odom_frame": "chassis_odom",
                "base_frame": "base_footprint",
            },
        ],
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
                "map",
                default_value=os.path.join(WORKSPACE_ROOT, "real_maps", "real_map11.yaml"),
                description="Map11 YAML file",
            ),
            DeclareLaunchArgument(
                "prior_pcd_file",
                default_value=os.path.join(
                    WORKSPACE_ROOT, "real_maps", "real_scans11.pcd"
                ),
                description="Map11 prior point cloud for small_gicp",
            ),
            DeclareLaunchArgument(
                "params_file",
                default_value=os.path.join(
                    pb_bringup_pkg, "config", "reality", "nav2_params.yaml"
                ),
                description="pb2025 real-car navigation parameters",
            ),
            DeclareLaunchArgument(
                "chassis_params_file",
                default_value=os.path.join(
                    sentry_bringup_pkg, "config", "chassis_bridge_nav.yaml"
                ),
                description="A-board chassis bridge parameters",
            ),
            DeclareLaunchArgument(
                "serial_port",
                default_value="/dev/ttyABoard",
                description="A-board serial device",
            ),
            DeclareLaunchArgument(
                "use_rviz",
                default_value="True",
                description="Start RViz with the pb2025 navigation view",
            ),
            DeclareLaunchArgument(
                "use_composition",
                default_value="False",
                description="Use independent processes by default for easier debugging",
            ),
            front_mid360_tf,
            base_link_tf,
            gimbal_yaw_tf,
            gimbal_yaw_fake_tf,
            pb2025_navigation,
            chassis_bridge,
        ]
    )
