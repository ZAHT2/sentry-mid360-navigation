"""手持 Mid360 平放建图启动文件（无底盘，使用内置 IMU）"""
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    bringup_pkg = get_package_share_directory("sentry_bringup")
    cartographer_config_dir = os.path.join(bringup_pkg, "config")

    use_sim_time = LaunchConfiguration("use_sim_time", default="false")

    # 静态 TF：lidar_link 和 imu_link 在 Mid360 中近似同一位置
    static_tf_imu = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="imu_to_lidar_tf",
        arguments=["0", "0", "0", "0", "0", "0", "lidar_link", "imu_link"],
    )

    # 静态 TF：给 lidar_link 一个根帧（手持时不需要 base_link）
    static_tf_base = Node(
        package="tf2_ros",
        executable="static_transform_publisher",
        name="base_to_lidar_tf",
        arguments=["0", "0", "1.0", "0", "0", "0", "base_link", "lidar_link"],
    )

    cartographer_node = Node(
        package="cartographer_ros",
        executable="cartographer_node",
        name="cartographer_node",
        output="screen",
        parameters=[{"use_sim_time": use_sim_time}],
        arguments=[
            "-configuration_directory", cartographer_config_dir,
            "-configuration_basename", "cartographer_handheld.lua",
        ],
        remappings=[
            ("points2", "/livox/lidar"),
            ("imu", "/livox/imu"),          # Mid360 内置 IMU
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
        static_tf_imu,
        static_tf_base,
        cartographer_node,
        occupancy_grid_node,
    ])
