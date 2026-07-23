"""Mid360 驱动 + PointCloud2→LaserScan 转换"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    bringup_pkg = get_package_share_directory("sentry_bringup")
    livox_pkg   = get_package_share_directory("livox_ros_driver2")

    mid360_config = os.path.join(bringup_pkg, "config", "mid360_config.json")
    pc2scan_config = os.path.join(bringup_pkg, "config", "pointcloud_to_laserscan.yaml")

    # ── 1. Mid360 驱动 ────────────────────────────────────────────────────────
    livox_driver = Node(
        package="livox_ros_driver2",
        executable="livox_ros_driver2_node",
        name="livox_lidar_publisher",
        output="screen",
        parameters=[{
            "user_config_path": mid360_config,
            "xfer_format": 0,          # 0=pointcloud2, 1=customMsg
            "multi_topic": 0,
            "data_src": 0,             # 0=实机, 1=仿真
            "publish_freq": 10.0,
            "output_data_type": 0,
            "frame_id": "lidar_link",
        }],
    )

    # ── 2. PointCloud2 → LaserScan（切 2D 层给 slam_toolbox）────────────────
    pc_to_scan = Node(
        package="pointcloud_to_laserscan",
        executable="pointcloud_to_laserscan_node",
        name="pointcloud_to_laserscan",
        remappings=[("cloud_in", "/livox/lidar")],
        parameters=[pc2scan_config],
        output="screen",
    )

    return LaunchDescription([
        livox_driver,
        pc_to_scan,
    ])
