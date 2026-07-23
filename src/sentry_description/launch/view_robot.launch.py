"""验证用：在 RViz2 中显示 URDF 模型"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command
from launch_ros.actions import Node


def generate_launch_description():
    pkg = get_package_share_directory("sentry_description")
    urdf_file = os.path.join(pkg, "urdf", "sentry.urdf.xacro")

    robot_description = Command(["xacro ", urdf_file])

    return LaunchDescription([
        Node(
            package="robot_state_publisher",
            executable="robot_state_publisher",
            parameters=[{"robot_description": robot_description}],
        ),
        Node(
            package="joint_state_publisher_gui",
            executable="joint_state_publisher_gui",
        ),
        Node(
            package="rviz2",
            executable="rviz2",
            arguments=["-d", os.path.join(pkg, "config", "view_robot.rviz")],
        ),
    ])
