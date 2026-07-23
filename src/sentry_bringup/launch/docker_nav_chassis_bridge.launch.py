"""Static real-car TF + chassis serial bridge for Docker pb2025 navigation.

Run the Docker pb2025 navigation stack separately. This launch only publishes the
real-car static TFs and connects Nav2 /cmd_vel to the verified A-board protocol.
"""

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def _without_conda_libs(value):
    return ":".join(
        path for path in value.split(":")
        if path and "anaconda" not in path and "conda" not in path
    )


def generate_launch_description():
    bringup_pkg = get_package_share_directory("sentry_bringup")
    default_params = os.path.join(bringup_pkg, "config", "chassis_bridge_nav.yaml")

    params_file = LaunchConfiguration("params_file")
    serial_port = LaunchConfiguration("serial_port")
    cmd_vel_topic = LaunchConfiguration("cmd_vel_topic")

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

    chassis_bridge = Node(
        package="sentry_hardware",
        executable="chassis_bridge_node",
        name="chassis_bridge_node",
        output="screen",
        additional_env={
            "LD_LIBRARY_PATH": _without_conda_libs(os.environ.get("LD_LIBRARY_PATH", "")),
        },
        parameters=[
            params_file,
            {
                "serial_port": serial_port,
                "cmd_vel_topic": cmd_vel_topic,
                "publish_tf": False,
                "odom_topic": "/chassis/odom_raw",
                "odom_frame": "chassis_odom",
                "base_frame": "base_footprint",
            },
        ],
    )

    return LaunchDescription([
        DeclareLaunchArgument(
            "params_file",
            default_value=default_params,
            description="YAML parameters for sentry_hardware/chassis_bridge_node",
        ),
        DeclareLaunchArgument(
            "serial_port",
            default_value="/dev/ttyABoard",
            description="A-board serial device, e.g. /dev/ttyABoard or /dev/ttyACM0",
        ),
        DeclareLaunchArgument(
            "cmd_vel_topic",
            default_value="/cmd_vel",
            description="Final velocity topic from Docker fake_vel_transform",
        ),
        front_mid360_tf,
        gimbal_yaw_tf,
        gimbal_yaw_fake_tf,
        chassis_bridge,
    ])
