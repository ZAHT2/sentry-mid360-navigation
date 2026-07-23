"""整车启动入口：robot_state_publisher + ros2_control + controller spawner"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, RegisterEventHandler, TimerAction
from launch.event_handlers import OnProcessStart
from launch.substitutions import LaunchConfiguration, Command
from launch_ros.actions import Node


def generate_launch_description():
    desc_pkg = get_package_share_directory("sentry_description")
    bringup_pkg = get_package_share_directory("sentry_bringup")

    urdf_file = os.path.join(desc_pkg, "urdf", "sentry.urdf.xacro")
    controllers_yaml = os.path.join(desc_pkg, "config", "ros2_controllers.yaml")

    robot_description = Command(["xacro ", urdf_file])

    # ── 1. robot_state_publisher ──────────────────────────────────────────────
    rsp = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": robot_description}],
        output="screen",
    )

    # ── 2. controller_manager（加载硬件接口插件）─────────────────────────────
    controller_manager = Node(
        package="controller_manager",
        executable="ros2_control_node",
        parameters=[
            {"robot_description": robot_description},
            controllers_yaml,
        ],
        output="screen",
    )

    # ── 3. joint_state_broadcaster（controller_manager 启动后 1s 再 spawn）──
    joint_state_broadcaster_spawner = TimerAction(
        period=1.0,
        actions=[
            Node(
                package="controller_manager",
                executable="spawner",
                arguments=["joint_state_broadcaster", "--controller-manager", "/controller_manager"],
                output="screen",
            )
        ],
    )

    return LaunchDescription([
        rsp,
        controller_manager,
        joint_state_broadcaster_spawner,
    ])
